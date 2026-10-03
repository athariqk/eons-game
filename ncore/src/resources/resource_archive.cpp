#include <cstddef>
#include <cstdint>
#include <fstream>

#include <ncore/core/collection.h>
#include <ncore/resources/resource_archive.h>
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
constexpr uint32_t kMaxArrayCount = 1u << 28; // 268M elements
constexpr uint32_t kMaxStringLength = 1u << 26; // 64M chars

Ref<IResource> make_resource( uint32_t format_id )
{
    if (format_id == ResourceFormatID( "nshd" ).id)
        return Ref<Shader>::create();
    if (format_id == ResourceFormatID( "nmat" ).id)
        return Ref<MaterialShader>::create();
    if (format_id == ResourceFormatID( "nimg" ).id)
        return Ref<Image>::create();
    if (format_id == ResourceFormatID( "naud" ).id)
        return Ref<AudioClip>::create();
    if (format_id == ResourceFormatID( "nfnt" ).id)
        return Ref<Font>::create();
    return nullptr;
}

bool known_format( uint32_t format_id )
{
    return format_id == ResourceFormatID( "nshd" ).id || format_id == ResourceFormatID( "nmat" ).id ||
           format_id == ResourceFormatID( "nimg" ).id || format_id == ResourceFormatID( "naud" ).id ||
           format_id == ResourceFormatID( "nfnt" ).id;
}

String format_id_string( uint32_t id )
{
    ResourceFormatID fid;
    fid.id = id;
    return fid.to_string();
}

// ----------------------------------------------------------------------------
// Writer: appends fields in RTTI visitation order. The header (and its
// size_bytes patch) is handled by serialize(); this visitor only emits the
// payload.
// ----------------------------------------------------------------------------
struct BinaryWriterVisitor final : public rtti::RecordVisitor {
    std::ofstream& out;
    bool ok = true;

    explicit BinaryWriterVisitor( std::ofstream& p_out [[clang::lifetimebound]] ) : out( p_out ) {}

    void class_begin( const rtti::RecordInfo*, int ) override {}
    void class_end( const rtti::RecordInfo*, int ) override {}
    void class_member( const rtti::FieldInfo*, int ) override {}

    void array_begin( const rtti::TypeInfo*, const rtti::RecordInfo*, void*, int, int length ) override
    {
        const uint32_t count = static_cast<uint32_t>( length );
        out.write( reinterpret_cast<const char*>( &count ), sizeof( count ) );
        if (!out)
            ok = false;
    }
    void array_end( const rtti::TypeInfo*, int ) override {}
    void array_element( const rtti::TypeInfo*, int, int ) override {}

    void primitive( const rtti::TypeInfo* t, void* instance ) override
    {
        if (!instance) {
            ok = false; // would desync the stream — fail loudly
            return;
        }
        out.write( static_cast<const char*>( instance ), static_cast<std::streamsize>( t->size ) );
        if (!out)
            ok = false;
    }

    void string( const rtti::TypeInfo*, void* instance ) override
    {
        if (!instance) {
            ok = false;
            return;
        }
        const auto* str     = static_cast<const String*>( instance );
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
struct BinaryReaderVisitor final : public rtti::RecordVisitor {
    std::ifstream& in;
    bool ok = true;

    explicit BinaryReaderVisitor( std::ifstream& p_in [[clang::lifetimebound]] ) : in( p_in ) {}

    void class_begin( const rtti::RecordInfo*, int ) override {}
    void class_end( const rtti::RecordInfo*, int ) override {}
    void class_member( const rtti::FieldInfo*, int ) override {}

    void array_begin( const rtti::TypeInfo*, const rtti::RecordInfo* container_type, void* container, int, int ) override
    {
        uint32_t count = 0;
        in.read( reinterpret_cast<char*>( &count ), sizeof( count ) );
        if (!in || count > kMaxArrayCount) {
            ok = false;
            return;
        }
        if (container_type && container)
            container_type->resize( container, count );
    }
    void array_end( const rtti::TypeInfo*, int ) override {}
    void array_element( const rtti::TypeInfo*, int, int ) override {}

    void primitive( const rtti::TypeInfo* t, void* instance ) override
    {
        if (!instance) {
            ok = false;
            return;
        }
        in.read( static_cast<char*>( instance ), static_cast<std::streamsize>( t->size ) );
        if (!in)
            ok = false;
    }

    void string( const rtti::TypeInfo*, void* instance ) override
    {
        if (!instance) {
            ok = false;
            return;
        }
        uint32_t length = 0;
        in.read( reinterpret_cast<char*>( &length ), sizeof( length ) );
        if (!in || length > kMaxStringLength) {
            ok = false;
            return;
        }

        String buffer;
        if (length > 0) {
            buffer.resize( length );
            in.read( buffer.data(), length );
            if (!in) {
                ok = false;
                return;
            }
        }
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
        NC_LOG_ERROR_C(
            log::IO, "serialize: '{}' has no valid format id / version", p_resource->get_class_name()
        );
        return Error::ERR_INVALID_PARAMETER;
    }

    out.write( reinterpret_cast<const char*>( &header ), sizeof( header ) );

    const std::streampos body_start = out.tellp();

    // Dispatch on the CONCRETE type: get_class_info() is virtual (NCLASS), so
    // subclasses serialize their own fields, parent-first.
    BinaryWriterVisitor visitor( out );
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

Ref<IResource> ResourceArchive::deserialize( const String& input_path )
{
    std::ifstream in( input_path.c_str(), std::ios::binary );
    if (!in.is_open()) {
        NC_LOG_ERROR_C( log::IO, "deserialize: cannot open '{}'", input_path );
        return nullptr;
    }

    DiskHeader header{};
    in.read( reinterpret_cast<char*>( &header ), sizeof( header ) );
    if (!in) {
        NC_LOG_ERROR_C( log::IO, "deserialize: '{}' truncated header", input_path );
        return nullptr;
    }

    if (header.format_id == 0 || header.version == 0) {
        NC_LOG_ERROR_C( log::IO, "deserialize: '{}' has invalid header (format_id={})", input_path, header.format_id );
        return nullptr;
    }

    if (!known_format( header.format_id )) {
        NC_LOG_ERROR_C(
            log::IO, "deserialize: '{}' has unknown format id '{}'", input_path, format_id_string( header.format_id )
        );
        return nullptr;
    }

    Ref<IResource> resource = make_resource( header.format_id );
    if (!resource) {
        return nullptr;
    }

    if (header.version != resource->get_version()) {
        NC_LOG_ERROR_C(
            log::IO, "deserialize: '{}' version {} unsupported (resource is {})", input_path, header.version,
            resource->get_version()
        );
        return nullptr;
    }

    const std::streampos body_start = in.tellg();

    BinaryReaderVisitor visitor( in );
    resource->get_class_info().visit( resource.get(), &visitor );

    if (!visitor.ok || !in.good()) {
        NC_LOG_ERROR_C( log::IO, "deserialize: '{}' payload read failed", input_path );
        return nullptr;
    }

    const std::streampos body_end = in.tellg();
    const uint64_t consumed =
        ( body_end >= body_start ) ? static_cast<uint64_t>( body_end - body_start ) : 0;
    if (consumed != header.size_bytes) {
        NC_LOG_ERROR_C(
            log::IO, "deserialize: '{}' payload size mismatch (header {} bytes, read {} bytes)", input_path,
            header.size_bytes, consumed
        );
        return nullptr;
    }

    // Rebuild derived (non-REFLECT) state — e.g. Shader::build_configs_().
    resource->on_deserialized();

    NC_LOG_DEBUG_C(
        log::IO, "deserialize: '{}' -> {} ({} bytes)", input_path, resource->get_class_name(), header.size_bytes
    );
    return resource;
}

} // namespace nc
