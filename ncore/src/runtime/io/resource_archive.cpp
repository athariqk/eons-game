// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: resource_archive.cpp

#include <cstddef>
#include <cstdint>
#include <fstream>

#include <ncore/core/collection.h>
#include <ncore/runtime/io/resource_archive.h>
#include <ncore/runtime/io/resource_loader.h>
#include <ncore/runtime/resources/audio_clip.h>
#include <ncore/runtime/resources/cubemap.h>
#include <ncore/runtime/resources/font.h>
#include <ncore/runtime/resources/image.h>
#include <ncore/runtime/resources/material.h>
#include <ncore/runtime/resources/mesh.h>
#include <ncore/runtime/resources/shader.h>
#include <ncore/utils/log.h>

namespace nc {

namespace {

// On-disk header: [format_id u32][version u32][size_bytes u64] = 16 bytes LE.
struct DiskHeader {
    uint32_t format_id  = 0;
    uint32_t version    = 0;
    uint64_t size_bytes = 0;
};
static_assert( sizeof( DiskHeader ) == 16, "resource header must be 16 bytes" );

// Sanity caps for corrupt-file protection while reading.
constexpr uint32_t MAX_ARRAY_COUNT   = 1u << 28; // 268M elements
constexpr uint32_t MAX_STRING_LENGTH = 1u << 26; // 64M chars

// Ref field encoding. A handle is never written as raw pointer bytes: it is a
// stable identity that ResourceLoader can resolve back into a live object.
enum RefKind : uint8_t {
    REF_NULL = 0, // empty handle, or a resource with no persistable identity
    REF_PATH = 1, // [u8][u32 length][bytes] asset-relative path (cross-session)
    REF_RID  = 2, // [u8][u64] loader RID (process-local fallback)
};

// True for the sentinel a programmatically created IResource carries.
bool is_generated_path( StringView p_path )
{
    return p_path.empty() || p_path == "generated";
}

// TODO: find a better pattern that is more extensible and proper
Ref<IResource> make_resource( uint32_t format_id )
{
    if (format_id == ResourceFormatID( "nshd" ).id)
        return Ref<Shader>::create();
    if (format_id == ResourceFormatID( "nmts" ).id)
        return Ref<MaterialShader>::create();
    if (format_id == ResourceFormatID( "nmat" ).id)
        return Ref<Material>::create();
    if (format_id == ResourceFormatID( "nmsh" ).id)
        return Ref<Mesh>::create();
    if (format_id == ResourceFormatID( "nimg" ).id)
        return Ref<Image>::create();
    if (format_id == ResourceFormatID( "ncbm" ).id)
        return Ref<CubeMap>::create();
    if (format_id == ResourceFormatID( "naud" ).id)
        return Ref<AudioClip>::create();
    if (format_id == ResourceFormatID( "nfnt" ).id)
        return Ref<Font>::create();
    return nullptr;
}

bool known_format( uint32_t format_id )
{
    return format_id == ResourceFormatID( "nshd" ).id || format_id == ResourceFormatID( "nmts" ).id ||
           format_id == ResourceFormatID( "nmat" ).id || format_id == ResourceFormatID( "nmsh" ).id ||
           format_id == ResourceFormatID( "nimg" ).id || format_id == ResourceFormatID( "ncbm" ).id ||
           format_id == ResourceFormatID( "naud" ).id || format_id == ResourceFormatID( "nfnt" ).id;
}

String format_id_string( uint32_t id )
{
    ResourceFormatID fid;
    fid.id = id;
    return fid.to_string();
}

struct ArchiveTrace {
    bool ok = true;
    std::string error;                   // first failure only; later ones are fallout

    std::vector<std::string_view> scope; // class_begin/class_end stack
    const types::FieldInfo* field = nullptr;
    int elem                      = -1;

    std::string where() const
    {
        std::string s;
        for (auto n : scope) {
            if (!s.empty())
                s += " > ";
            s += n;
        }
        if (field)
            s += std::format( ".{}", std::string_view( field->name.data(), field->name.size() ) );
        if (elem >= 0)
            s += std::format( "[{}]", elem );
        return s.empty() ? "<root>" : s;
    }

    template<typename... Args>
    void fail( std::format_string<Args...> fmt, Args&&... args )
    {
        if (!ok)
            return; // keep the root cause
        ok    = false;
        error = std::format( "at {}: ", where() ) + std::format( fmt, std::forward<Args>( args )... );
    }
};

// ----------------------------------------------------------------------------
// Writer: appends fields in RTTI visitation order. The header (and its
// size_bytes patch) is handled by serialize(); this visitor only emits the
// payload.
// ----------------------------------------------------------------------------
struct BinaryWriterVisitor final : public types::RecordVisitor {
    std::ofstream& out;
    ResourceLoader* loader;
    bool ok = true;

    BinaryWriterVisitor( std::ofstream& p_out NC_LIFETIME_BOUND, ResourceLoader* p_loader ) :
        out( p_out ), loader( p_loader )
    {}

    void put_u8( uint8_t v )
    {
        out.write( reinterpret_cast<const char*>( &v ), sizeof( v ) );
        if (!out)
            ok = false;
    }
    void put_u32( uint32_t v )
    {
        out.write( reinterpret_cast<const char*>( &v ), sizeof( v ) );
        if (!out)
            ok = false;
    }
    void put_u64( uint64_t v )
    {
        out.write( reinterpret_cast<const char*>( &v ), sizeof( v ) );
        if (!out)
            ok = false;
    }

    void class_begin( const types::RecordInfo*, int ) override {}
    void class_end( const types::RecordInfo*, int ) override {}
    void class_member( const types::FieldInfo*, int ) override {}

    /**
     * @brief Persist a Ref<T> slot as a path, a loader RID, or null.
     *
     * Path wins because it is the only identity that survives a restart. The
     * loader RID is the fallback for resources that were created in memory and
     * never had a file. Non-IResource handles are not persisted yet.
     */
    void reference( const types::RecordInfo* c, void* instance ) override
    {
        if (!instance) {
            ok = false;
            return;
        }
        const types::IRefOps* ops = c->as_ref();
        if (!ops) {
            ok = false;
            return;
        }

        RefCounted* rc = ops->get_refcounted( instance );
        IResource* res = rc && rc->is_a<IResource>() ? static_cast<IResource*>( rc ) : nullptr;
        if (!res) {
            put_u8( REF_NULL );
            if (rc)
                NC_LOG_WARN_C( log::IO, "serialize: ref '{}' is not an IResource; persisted as null", c->name );
            return;
        }

        const StringView path = res->filepath;
        if (!is_generated_path( path )) {
            put_u8( REF_PATH );
            const uint32_t length = static_cast<uint32_t>( path.size() );
            put_u32( length );
            if (length > 0) {
                out.write( path.data(), length );
                if (!out)
                    ok = false;
            }
            return;
        }

        const RID handle = loader ? loader->find_handle( res ) : RID();
        if (handle.is_valid()) {
            put_u8( REF_RID );
            put_u64( handle.value );
            return;
        }

        put_u8( REF_NULL );
        NC_LOG_WARN_C(
            log::IO, "serialize: '{}' has no asset path or loaded RID; persisted as null", res->get_class_name()
        );
    }

    void array_begin( const types::TypeInfo*, const types::RecordInfo*, void*, int, int length ) override
    {
        const uint32_t count = static_cast<uint32_t>( length );
        out.write( reinterpret_cast<const char*>( &count ), sizeof( count ) );
        if (!out)
            ok = false;
    }
    void array_end( const types::TypeInfo*, int ) override {}
    void array_element( const types::TypeInfo*, int, int ) override {}

    void primitive( const types::TypeInfo* t, void* instance ) override
    {
        if (!instance) {
            ok = false; // would desync the stream — fail loudly
            return;
        }
        out.write( static_cast<const char*>( instance ), static_cast<std::streamsize>( t->size ) );
        if (!out)
            ok = false;
    }

    void primitive_array( const types::TypeInfo* elem_type, void* data, size_t count ) override
    {
        if (count == 0)
            return;
        if (!data || !elem_type) {
            ok = false;
            return;
        }
        out.write( static_cast<const char*>( data ), static_cast<std::streamsize>( count * elem_type->size ) );
        if (!out)
            ok = false;
    }

    void string( const types::TypeInfo*, void* instance ) override
    {
        if (!instance) {
            ok = false;
            return;
        }
        const auto* str       = static_cast<const String*>( instance );
        const uint32_t length = static_cast<uint32_t>( str->length() );
        out.write( reinterpret_cast<const char*>( &length ), sizeof( length ) );
        if (length > 0)
            out.write( str->c_str(), length );
        if (!out)
            ok = false;
    }
};

// ----------------------------------------------------------------------------
// Reader: mirrors BinaryWriterVisitor. Resizes containers (VectorClass) from
// the count stored in the stream before element visitation.
// ----------------------------------------------------------------------------
struct BinaryReaderVisitor final : public types::RecordVisitor, ArchiveTrace {
    std::ifstream& in;
    ResourceLoader* loader;

    BinaryReaderVisitor( std::ifstream& p_in NC_LIFETIME_BOUND, ResourceLoader* p_loader ) :
        in( p_in ), loader( p_loader )
    {}

    bool read_raw( void* dst, size_t n, const char* what )
    {
        if (!ok)
            return false;
        const auto pos = static_cast<int64_t>( in.tellg() );
        in.read( static_cast<char*>( dst ), static_cast<std::streamsize>( n ) );
        if (!in) {
            fail(
                "reading {} ({} bytes at file offset {}): got {} bytes, {}", what, n, pos, in.gcount(),
                in.eof() ? "unexpected end of file" : "stream error"
            );
            return false;
        }
        return true;
    }

    uint8_t get_u8()
    {
        uint8_t v = 0;
        read_raw( &v, sizeof v, "u8" );
        return v;
    }
    uint32_t get_u32()
    {
        uint32_t v = 0;
        read_raw( &v, sizeof v, "u32" );
        return v;
    }
    uint64_t get_u64()
    {
        uint64_t v = 0;
        read_raw( &v, sizeof v, "u64" );
        return v;
    }

    void class_begin( const types::RecordInfo* c, int ) override
    {
        scope.push_back( c->name );
        field = nullptr;
        elem  = -1;
    }
    void class_end( const types::RecordInfo*, int ) override
    {
        scope.pop_back();
        field = nullptr;
        elem  = -1;
    }
    void class_member( const types::FieldInfo* f, int ) override
    {
        field = f;
        elem  = -1;
    }

    /**
     * @brief Resolve a serialized Ref<T> slot back into a live handle.
     *
     * An unresolvable dependency clears the slot and warns: keeping the owning
     * resource loadable beats failing the whole file over one missing link.
     */
    void reference( const types::RecordInfo* c, void* instance ) override
    {
        const types::IRefOps* ops = c->as_ref();
        if (!ops || !instance) {
            fail( "ref '{}' has no IRefOps or null slot", c->name );
            return;
        }

        const uint8_t kind = get_u8();
        if (!ok)
            return;
        if (kind == REF_NULL) {
            ops->reset( instance );
            return;
        }

        Ref<IResource> resolved;
        String path;
        if (kind == REF_PATH) {
            const uint32_t length = get_u32();
            if (!ok)
                return;
            if (length > MAX_STRING_LENGTH) {
                fail( "ref path length {} exceeds limit {}", length, MAX_STRING_LENGTH );
                return;
            }
            path.resize( length );
            if (length > 0 && !read_raw( path.data(), length, "ref path" ))
                return;
            if (loader) {
                const RID handle = loader->load( path );
                if (handle.is_valid())
                    resolved = loader->get( handle );
            }
        } else if (kind == REF_RID) {
            const uint64_t value = get_u64();
            if (!ok)
                return;
            if (loader)
                resolved = loader->get( RID{ value } );
        } else {
            fail( "unknown ref kind {} (corrupt stream or format mismatch)", kind );
            return;
        }

        if (!resolved) {
            NC_LOG_WARN_C(
                log::IO, "unresolved ref '{}' ({}) at {}; leaving it empty", c->name,
                kind == REF_PATH ? path.c_str() : "RID", where()
            );
            ops->reset( instance );
            return;
        }
        if (!ops->share( instance, resolved.get() )) {
            NC_LOG_WARN_C(
                log::IO, "ref '{}' at {} resolved to '{}', wrong type; leaving it empty", c->name, where(),
                resolved->get_class_name()
            );
            ops->reset( instance );
        }
    }

    void
    array_begin( const types::TypeInfo*, const types::RecordInfo* container_type, void* container, int, int ) override
    {
        const uint32_t count = get_u32();
        if (!ok)
            return;
        if (count > MAX_ARRAY_COUNT) {
            fail( "array count {} exceeds limit {} (corrupt or out-of-sync stream)", count, MAX_ARRAY_COUNT );
            return;
        }
        if (container_type && container)
            container_type->resize( container, count );
    }

    void array_end( const types::TypeInfo*, int ) override {}
    void array_element( const types::TypeInfo*, int, int i ) override
    {
        elem = i;
    }

    void primitive( const types::TypeInfo* t, void* instance ) override
    {
        if (!instance) {
            fail( "null slot for primitive '{}'", t->name );
            return;
        }
        read_raw( instance, t->size, t->name );
    }

    void primitive_array( const types::TypeInfo* e, void* data, size_t count ) override
    {
        if (count == 0)
            return;
        if (!data || !e) {
            fail( "null buffer for primitive array" );
            return;
        }
        read_raw( data, count * e->size, "primitive array" );
    }

    void string( const types::TypeInfo*, void* instance ) override
    {
        if (!instance) {
            fail( "null slot for string" );
            return;
        }
        const uint32_t length = get_u32();
        if (!ok)
            return;
        if (length > MAX_STRING_LENGTH) {
            fail( "string length {} exceeds limit {} (corrupt or out-of-sync stream)", length, MAX_STRING_LENGTH );
            return;
        }
        String buffer;
        buffer.resize( length );
        if (length > 0 && !read_raw( buffer.data(), length, "string bytes" ))
            return;
        *static_cast<String*>( instance ) = std::move( buffer );
    }
};

} // namespace

Error ResourceArchive::serialize( const Ref<IResource>& p_resource, const String& p_output_path ) const
{
    if (!p_resource) {
        return Error::ERR_INVALID_PARAMETER;
    }

    std::ofstream out( p_output_path.c_str(), std::ios::binary | std::ios::trunc );
    if (!out.is_open()) {
        return Error::ERR_FILE_CANT_WRITE;
    }

    DiskHeader header;
    header.format_id  = p_resource->get_format_id().id;
    header.version    = p_resource->get_version();
    header.size_bytes = 0; // patched once the body has been written

    if (header.format_id == 0 || header.version == 0) {
        NC_LOG_ERROR_C( log::IO, "'{}' has no valid format id / version", p_resource->get_class_name() );
        return Error::ERR_INVALID_PARAMETER;
    }

    out.write( reinterpret_cast<const char*>( &header ), sizeof( header ) );

    const std::streampos body_start = out.tellp();

    // Dispatch on the CONCRETE type: get_class_info() is virtual (NCLASS), so
    // subclasses serialize their own fields, parent-first.
    BinaryWriterVisitor visitor( out, loader );
    p_resource->get_class_info().visit( const_cast<IResource*>( p_resource.get() ), &visitor );

    if (!visitor.ok || !out.good()) {
        return Error::ERR_FILE_CANT_WRITE;
    }

    const std::streampos body_end = out.tellp();
    header.size_bytes             = static_cast<uint64_t>( body_end - body_start );

    out.seekp( static_cast<std::streamoff>( offsetof( DiskHeader, size_bytes ) ) );
    out.write( reinterpret_cast<const char*>( &header.size_bytes ), sizeof( header.size_bytes ) );

    const bool ok = out.good();
    out.close();
    return ok ? Error::OK : Error::ERR_FILE_CANT_WRITE;
}

Ref<IResource> ResourceArchive::deserialize( const String& input_path, ResourceLoader* p_loader )
{
    std::ifstream in( input_path.c_str(), std::ios::binary );
    if (!in.is_open()) {
        NC_LOG_ERROR_C( log::IO, "Cannot open '{}'", input_path );
        return nullptr;
    }

    DiskHeader header{};
    in.read( reinterpret_cast<char*>( &header ), sizeof( header ) );
    if (!in) {
        NC_LOG_ERROR_C( log::IO, "'{}' truncated header", input_path );
        return nullptr;
    }

    if (header.format_id == 0 || header.version == 0) {
        NC_LOG_ERROR_C( log::IO, "'{}' has invalid header (format_id={})", input_path, header.format_id );
        return nullptr;
    }

    if (!known_format( header.format_id )) {
        NC_LOG_ERROR_C( log::IO, "'{}' has unknown format id '{}'", input_path, format_id_string( header.format_id ) );
        return nullptr;
    }

    Ref<IResource> resource = make_resource( header.format_id );
    if (!resource) {
        return nullptr;
    }

    if (header.version != resource->get_version()) {
        NC_LOG_ERROR_C(
            log::IO, "'{}' version {} unsupported (resource is {})", input_path, header.version, resource->get_version()
        );
        return nullptr;
    }

    in.seekg( 0, std::ios::end );
    const auto file_size = static_cast<uint64_t>( in.tellg() );
    in.seekg( sizeof( DiskHeader ), std::ios::beg );
    if (file_size < sizeof( DiskHeader ) + header.size_bytes) {
        NC_LOG_ERROR_C(
            log::IO, "'{}' truncated: header declares {} body bytes, file has only {}", input_path, header.size_bytes,
            file_size > sizeof( DiskHeader ) ? file_size - sizeof( DiskHeader ) : 0
        );
        return nullptr;
    }

    const std::streampos body_start = in.tellg();

    BinaryReaderVisitor visitor( in, p_loader );
    resource->get_class_info().visit( resource.get(), &visitor );

    if (!visitor.ok || !in.good()) {
        NC_LOG_ERROR_C(
            log::IO, "'{}' failed to load as {} (format '{}', v{}): {}", input_path, resource->get_class_name(),
            format_id_string( header.format_id ), header.version,
            visitor.ok ? "stream left in a bad state after visit" : visitor.error.c_str()
        );
        return nullptr;
    }

    const std::streampos body_end = in.tellg();
    const uint64_t consumed       = ( body_end >= body_start ) ? static_cast<uint64_t>( body_end - body_start ) : 0;
    if (consumed != header.size_bytes) {
        NC_LOG_ERROR_C(
            log::IO, "'{}' payload size mismatch (header {} bytes, read {} bytes)", input_path, header.size_bytes,
            consumed
        );
        return nullptr;
    }

    // Rebuild derived (non-REFLECT) state — e.g. Shader::build_configs().
    resource->on_deserialized();

    NC_LOG_DEBUG_C(
        log::IO, "Deserialized '{}' -> {} ({} bytes)", input_path, resource->get_class_name(), header.size_bytes
    );
    return resource;
}

} // namespace nc
