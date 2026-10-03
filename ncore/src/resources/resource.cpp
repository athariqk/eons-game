#include <ncore/resources/resource.h>
#include <ncore/services/video/renderer/vertex_format.h>
#include <ncore/utils/four_cc.h>
#include <ncore/utils/log.h>

namespace nc {

ResourceFormatID::ResourceFormatID( StringView ascii_id )
{
    id = FourCC::from_string( ascii_id );
}

ResourceFormatID::ResourceFormatID( const char* ascii_id )
{
    id = FourCC::from_string( ascii_id );
}

bool ResourceFormatID::is_valid() const
{
    return id != 0 && FourCC::is_valid_fourcc( id );
}

String ResourceFormatID::to_string() const
{
    return is_valid() ? FourCC::to_string( id ) : String();
}

//------------------------------------------------------------------------------

Shader::Shader( const ShaderDesc& p_desc )
{
    stages.push_back( p_desc );
    build_configs_();
}

Shader::Shader( DynamicArray<ShaderDesc> p_stages_desc ) : stages( p_stages_desc )
{
    build_configs_();
}

ResourceFormatID Shader::get_format_id() const
{
    return "nshd";
}

void Shader::on_deserialized()
{
    build_configs_();
}

ResourceFormatID MaterialShader::get_format_id() const
{
    return "nmat";
}

size_t Shader::get_size_bytes() const
{
    size_t total = sizeof( Shader );
    for (const auto& stage : stages) {
        total += stage.bytecode.size() * sizeof( uint32_t );
        total += stage.params.size() * sizeof( ShaderParamDesc );
    }
    return total;
}

StringView Shader::get_entry_point( gfx::ShaderStage stage ) const
{
    if (stage == gfx::ShaderStage::NONE && !stages.empty()) {
        return stages[0].entrypoint;
    }
    if (auto desc = get_stage_desc( stage )) {
        return desc->entrypoint;
    }
    return {};
}

Span<const uint32_t> Shader::get_bytecode( gfx::ShaderStage stage ) const
{
    if (stage == gfx::ShaderStage::NONE && !stages.empty()) {
        return stages[0].bytecode;
    }
    if (auto desc = get_stage_desc( stage )) {
        return desc->bytecode;
    }
    return {};
}

Span<const ShaderParamDesc> Shader::get_params() const
{
    return unified_params;
}

Span<const ShaderParamDesc> Shader::get_param_set( gfx::BindingSet p_binding_set ) const
{
    auto it = param_set_lookup.find( p_binding_set );
    if (it != param_set_lookup.end()) {
        return it->second;
    }
    return {};
}

const ShaderParamDesc* Shader::find_param( StringView p_name ) const
{
    auto it = param_name_lookup.find( p_name );
    if (it != param_name_lookup.end()) {
        return &unified_params[it->second];
    }
    return nullptr;
}

bool Shader::has_stage( gfx::ShaderStage stage ) const
{
    return std::any_of( stages.begin(), stages.end(), [stage]( const ShaderDesc& s ) { return s.stage == stage; } );
}

const ShaderDesc* Shader::get_stage_desc( gfx::ShaderStage stage ) const
{
    auto it = std::find_if( stages.begin(), stages.end(), [stage]( const ShaderDesc& s ) { return s.stage == stage; } );
    return it != stages.end() ? it._Ptr : nullptr;
}

Span<const ShaderDesc> Shader::get_stages() const
{
    return stages;
}

gfx::ShaderStage Shader::get_stage_flags() const
{
    gfx::ShaderStage flags = gfx::ShaderStage::NONE;
    for (auto& desc : stages) {
        flags = flags | desc.stage;
    }
    return flags;
}

void Shader::build_configs_()
{
    // FIXME: this sucks. too complicated.
    // Algorithm:
    // - populate unified_params by adding over all params from all stages, using param_name_lookup hashmap for dedup.
    // - then we need to populate param_set_lookup, which is a span view over unified_params by BindingSet for memory
    // efficiency.
    // - that is not possible without sorting unified_params first, so that twe can have contiguous array ordered by
    // BindingSet.
    // - patch param_name_lookup indexes after sorting.
    // - populate param_set_lookup by constructing spans over a BindingSet subregion of unified_param.

    unified_params.clear();
    param_name_lookup.clear();

    for (const auto& stage_desc : stages) {
        for (const auto& param : stage_desc.params) {
            auto it = param_name_lookup.find( param.name );
            if (it != param_name_lookup.end()) {
                // parameter already exists: union stage mask
                auto& existing      = unified_params[it->second];
                existing.stage_mask = existing.stage_mask | stage_desc.stage;

                // A stage that does not use a parameter gets no descriptor binding for it
                // (the SPIR-V drops it), and reflection marks it with the unbound set
                // (0xFFFF, see kUnboundBindingSet in shader_compiler.cpp). Keep whichever
                // stage reported a real set, otherwise the merged param lands in set 0xFFFF
                // and never reaches the resource signature that the PSO needs.
                constexpr gfx::BindingSet kUnbound = 0xFFFFu;
                if (existing.binding_space == kUnbound && param.binding_space != kUnbound) {
                    existing.binding_space = param.binding_space;
                    existing.binding_idx   = param.binding_idx;
                }

                // Each stage reflects the block against its own SPIR-V. A stage whose
                // entry point dropped the block (or a stage that runs before the block
                // shows up in SPIR-V) reports the smaller layout, so keep the largest.
                if (param.total_size_bytes > existing.total_size_bytes)
                    existing.total_size_bytes = param.total_size_bytes;
            } else {
                // new parameter insertion
                ShaderParamDesc unified_param = param;
                unified_param.stage_mask      = static_cast<gfx::ShaderStage>( stage_desc.stage );

                size_t index = unified_params.size();
                unified_params.push_back( std::move( unified_param ) );
                param_name_lookup[param.name] = index;
            }
        }
    }

    std::stable_sort(
        unified_params.begin(), unified_params.end(),
        []( const ShaderParamDesc& a, const ShaderParamDesc& b ) -> bool { return a.binding_space < b.binding_space; }
    );

    // indices captured in param_name_lookup during the insertion pass are now
    // invalid because the sort permuted unified_params, we need to rebuild it
    param_name_lookup.clear();
    for (size_t idx = 0; idx < unified_params.size(); ++idx) {
        param_name_lookup[unified_params[idx].name] = idx;
    }

    param_set_lookup.clear();
    size_t i = 0;
    while (i < unified_params.size()) {
        auto set           = unified_params[i].binding_space;
        const size_t start = i;
        while (i < unified_params.size() && unified_params[i].binding_space == set) {
            ++i;
        }
        param_set_lookup[set] = Span<const ShaderParamDesc>( unified_params.data() + start, i - start );
    }
}

//------------------------------------------------------------------------------

ResourceFormatID Mesh::get_format_id() const
{
    return "nmsh";
}

size_t Mesh::get_size_bytes() const
{
    return desc.vertices.size() + desc.indices.size();
}

Span<const std::byte> Mesh::get_vertices() const
{
    return desc.vertices;
}

Span<const uint16_t> Mesh::get_indices() const
{
    return desc.indices;
}

uint32_t Mesh::get_vertex_stride() const
{
    return desc.vertex_stride;
}

size_t Mesh::vertex_count() const
{
    return desc.vertex_stride ? desc.vertices.size() / desc.vertex_stride : 0;
}

size_t Mesh::index_count() const
{
    return desc.indices.size();
}

constexpr Array<Vertex3D, 24> CUBE_VERTS = {
    //  px,    py,    pz,    nx,    ny,    nz,    tx,   ty,   tz,   tw,   u,    v,    u2,   v2,   color
    Vertex3D{ -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },

    Vertex3D{ -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF },
    Vertex3D{ -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0xFFFFFFFF }
};
constexpr Array<uint16_t, 36> CUBE_INDICES = {
    // Front Face (Z = -1)
    2, 0, 1, 2, 3, 0,
    // Bottom Face (Y = -1)
    6, 4, 5, 6, 7, 4,
    // Right Face (X = +1)
    10, 8, 9, 10, 11, 8,
    // Top Face (Y = +1)
    14, 12, 13, 14, 15, 12,
    // Left Face (X = -1)
    18, 16, 17, 18, 19, 16,
    // Back Face (Z = +1)
    22, 20, 21, 22, 23, 20
};

CubeMesh::CubeMesh() :
    Mesh(
        MeshDesc{
            .vertices = DynamicArray<std::byte>(
                reinterpret_cast<std::byte const*>( CUBE_VERTS.data() ),
                reinterpret_cast<std::byte const*>( CUBE_VERTS.data() + CUBE_VERTS.size() )
            ),
            .indices       = DynamicArray<uint16_t>( CUBE_INDICES.data(), CUBE_INDICES.data() + CUBE_INDICES.size() ),
            .vertex_stride = sizeof( Vertex3D )
        }
    )
{}

PlaneMesh::PlaneMesh( uint32_t x_segments, uint32_t z_segments ) : Mesh( build_mesh_desc_( x_segments, z_segments ) ) {}

MeshDesc PlaneMesh::build_mesh_desc_( uint32_t x_segments, uint32_t z_segments )
{
    NC_ASSERT_MSG( x_segments >= 1 && z_segments >= 1, "PlaneMesh segments must be >= 1" );
    NC_ASSERT_MSG(
        x_segments <= 255 && z_segments <= 255, "PlaneMesh segments are limited to 255 per side (uint16 indices)"
    );

    const uint32_t vert_count = ( x_segments + 1 ) * ( z_segments + 1 );

    BytesBuffer vertices( static_cast<size_t>( vert_count ) * sizeof( Vertex3D ) );
    auto* verts = reinterpret_cast<Vertex3D*>( vertices.data() );

    for (uint32_t j = 0; j <= z_segments; j++) {
        for (uint32_t i = 0; i <= x_segments; i++) {
            const float x = -1.0f + 2.0f * ( static_cast<float>( i ) / static_cast<float>( x_segments ) );
            const float z = 1.0f - 2.0f * ( static_cast<float>( j ) / static_cast<float>( z_segments ) );
            const float u = static_cast<float>( i ) / static_cast<float>( x_segments );
            const float v = static_cast<float>( j ) / static_cast<float>( z_segments );

            verts[j * ( x_segments + 1 ) + i] =
                Vertex3D{ x, 0.0f, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, u, v, u, v, 0xFFFFFFFF };
        }
    }

    DynamicArray<uint16_t> indices;
    indices.reserve( static_cast<size_t>( x_segments ) * z_segments * 6 );

    for (uint32_t j = 0; j < z_segments; j++) {
        for (uint32_t i = 0; i < x_segments; i++) {
            const uint16_t v00 = static_cast<uint16_t>( j * ( x_segments + 1 ) + i );
            const uint16_t v01 = static_cast<uint16_t>( ( j + 1 ) * ( x_segments + 1 ) + i );
            const uint16_t v10 = static_cast<uint16_t>( j * ( x_segments + 1 ) + i + 1 );
            const uint16_t v11 = static_cast<uint16_t>( ( j + 1 ) * ( x_segments + 1 ) + i + 1 );

            // CCW from +Y (outward).
            indices.push_back( v00 );
            indices.push_back( v11 );
            indices.push_back( v01 );
            indices.push_back( v00 );
            indices.push_back( v10 );
            indices.push_back( v11 );
        }
    }

    return MeshDesc{
        .vertices = std::move( vertices ), .indices = std::move( indices ), .vertex_stride = sizeof( Vertex3D )
    };
}

//------------------------------------------------------------------------------

Image::Image( int w, int h, const void* rgba_pixels ) : width( w ), height( h )
{
    set_dimension( w, h );
    set_data( rgba_pixels );
}

ResourceFormatID Image::get_format_id() const
{
    return "nimg";
}

size_t Image::get_size_bytes() const
{
    return pixels.size();
}

uint32_t Image::get_width() const
{
    return width;
}

uint32_t Image::get_height() const
{
    return height;
}

Span<const std::byte> Image::get_pixels() const
{
    return pixels;
}

void* Image::get_raw()
{
    return pixels.data();
}

void Image::set_dimension( int w, int h )
{
    width  = w;
    height = h;
}

void Image::set_data( const void* rgba_pixels )
{
    NC_ASSERT( width > 0 && height > 0 );
    auto* p = static_cast<const std::byte*>( rgba_pixels );
    pixels.assign( p, p + static_cast<size_t>( width * height * 4 ) );
}

//------------------------------------------------------------------------------

struct RGBA8 {
    uint8_t channels[4];
};

static RGBA8 bilinear_sample_( const uint8_t* pixels, int w, int h, float eu, float ev )
{
    float fx = eu * static_cast<float>( w ) - 0.5f;
    float fy = ev * static_cast<float>( h ) - 0.5f;

    int x0   = static_cast<int>( std::floor( fx ) );
    int y0   = static_cast<int>( std::floor( fy ) );
    float tx = fx - std::floor( fx );
    float ty = fy - std::floor( fy );

    auto wrapped = [w]( int x ) { return ( ( x % w ) + w ) % w; };
    int x1       = wrapped( x0 + 1 );
    int y1       = std::clamp( y0 + 1, 0, h - 1 );
    x0           = wrapped( x0 );
    y0           = std::clamp( y0, 0, h - 1 );

    const uint8_t* p00 = pixels + ( static_cast<size_t>( y0 ) * w + x0 ) * 4;
    const uint8_t* p10 = pixels + ( static_cast<size_t>( y0 ) * w + x1 ) * 4;
    const uint8_t* p01 = pixels + ( static_cast<size_t>( y1 ) * w + x0 ) * 4;
    const uint8_t* p11 = pixels + ( static_cast<size_t>( y1 ) * w + x1 ) * 4;

    RGBA8 out;
    for (int c = 0; c < 4; c++) {
        float top       = p00[c] * ( 1.0f - tx ) + p10[c] * tx;
        float bottom    = p01[c] * ( 1.0f - tx ) + p11[c] * tx;
        out.channels[c] = static_cast<uint8_t>( top * ( 1.0f - ty ) + bottom * ty );
    }
    return out;
}

static Vec3 face_direction_( int face, float u, float v )
{
    switch (face) {
        case 0:
            return Vec3( 1.0f, v, -u );  // +X
        case 1:
            return Vec3( -1.0f, v, u );  // -X
        case 2:
            return Vec3( u, 1.0f, -v );  // +Y
        case 3:
            return Vec3( u, -1.0f, v );  // -Y
        case 4:
            return Vec3( u, v, 1.0f );   // +Z
        default:
            return Vec3( -u, v, -1.0f ); // -Z
    }
}

CubeMap::CubeMap( const Ref<Image>& equirect, uint32_t face_size )
{
    const int ew   = static_cast<int>( equirect->get_width() );
    const int eh   = static_cast<int>( equirect->get_height() );
    const auto src = reinterpret_cast<const uint8_t*>( equirect->get_pixels().data() );

    if (face_size <= 0) {
        face_size = equirect->get_width() / 4;
    }
    const uint32_t n = face_size;

    for (int face = 0; face < 6; face++) {
        DynamicArray<std::byte> buffer( static_cast<size_t>( n ) * n * 4 );
        auto* dst = reinterpret_cast<uint8_t*>( buffer.data() );

        for (uint32_t y = 0; y < n; y++) {
            for (uint32_t x = 0; x < n; x++) {
                const float u = ( 2.0f * static_cast<float>( x ) + 1.0f ) / static_cast<float>( n ) - 1.0f;
                const float v = 1.0f - ( 2.0f * static_cast<float>( y ) + 1.0f ) / static_cast<float>( n );

                Vec3 d = face_direction_( face, u, v ).normalize();

                const float eu = std::atan2( d.z, d.x ) / math::DOUBLE_PI + 0.5f;
                const float ev = std::acos( std::clamp( d.y, -1.0f, 1.0f ) ) / math::PI;

                auto sample      = bilinear_sample_( src, ew, eh, eu, ev );
                const size_t idx = ( static_cast<size_t>( y ) * n + x ) * 4;
                dst[idx + 0]     = sample.channels[0];
                dst[idx + 1]     = sample.channels[1];
                dst[idx + 2]     = sample.channels[2];
                dst[idx + 3]     = sample.channels[3];
            }
        }

        faces[face] = Ref<Image>::create( static_cast<int>( n ), static_cast<int>( n ), buffer.data() );
    }
}

ResourceFormatID CubeMap::get_format_id() const
{
    return "ncbm";
}

size_t CubeMap::get_size_bytes() const
{
    size_t total = 0;
    for (auto& face : faces) {
        total += face->get_size_bytes();
    }
    return total;
}

Span<const Ref<Image>, 6> CubeMap::get_faces() const
{
    return faces;
}

//------------------------------------------------------------------------------

AudioClip::AudioClip( const void* p_data, int p_length, int p_channels, int p_frequency, int p_bits_per_sample ) :
    length( p_length ), channels( p_channels ), frequency( p_frequency ), bits_per_sample( p_bits_per_sample )
{
    auto* p = static_cast<const std::byte*>( p_data );
    data.assign( p, p + static_cast<size_t>( p_length ) );
}

ResourceFormatID AudioClip::get_format_id() const
{
    return "naud";
}

Span<std::byte> AudioClip::get_data()
{
    return data;
}

Span<const std::byte> AudioClip::get_data() const
{
    return data;
}

int AudioClip::get_length() const
{
    return length;
}

int AudioClip::get_channels() const
{
    return channels;
}

int AudioClip::get_frequency() const
{
    return frequency;
}

int AudioClip::get_bits_per_sample() const
{
    return bits_per_sample;
}

size_t AudioClip::get_size_bytes() const
{
    return data.size();
}

ResourceFormatID Font::get_format_id() const
{
    return "nfnt";
}

Span<const std::byte> Font::get_data() const
{
    return data;
}

void Font::set_data( const void* p_data, size_t p_size )
{
    auto* p = static_cast<const std::byte*>( p_data );
    data.assign( p, p + p_size );
}

size_t Font::get_size_bytes() const
{
    return data.size();
}

} // namespace nc
