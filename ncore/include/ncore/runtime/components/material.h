#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>
#include <ncore/resources/material_shader.h>
#include <ncore/services/video/rhi_types.h>

namespace nc {

constexpr int MAXIMUM_MATERIAL_TEXTURE = 8;

/**
 * Scene material instance.
 * Source must be a Shader resource RID (not a MaterialTemplate).
 */
struct NCAPI MaterialComponent {
    RID Source   = 0; // Shader resource
    RID Instance = 0; // GPU material from material_create

    Array<RID, MAXIMUM_MATERIAL_TEXTURE> Textures = {};
    int TextureCount = 0;

    rhi::FillMode DrawMode = rhi::FillMode::SOLID;

    MaterialShaderFlags Flags = MaterialShaderFlags::None;
    String VertexLayoutName; // optional override; empty = VS reflection

    MaterialParameterStorage Params{};
    bool ParamsDirty = false;

    void add_texture( RID texture_rid )
    {
        NC_FAIL_MSG_RET( TextureCount < MAXIMUM_MATERIAL_TEXTURE, "Max texture count reached." );
        Textures[TextureCount++] = texture_rid;
    }

    void set_params_bytes( const void* data, uint32_t size )
    {
        NC_VERIFY( size <= MaterialParameterStorage::kMaxBytes );
        std::memcpy( Params.bytes, data, size );
        Params.size = size;
        ParamsDirty = true;
    }

    template <typename T>
    void set_params( const T& value )
    {
        static_assert( sizeof( T ) <= MaterialParameterStorage::kMaxBytes );
        set_params_bytes( &value, static_cast<uint32_t>( sizeof( T ) ) );
    }

    MaterialDrawBucket draw_bucket() const { return draw_bucket_from_flags( Flags ); }

    MaterialCreateDesc create_desc( StringView fallback_name = {} ) const
    {
        MaterialCreateDesc d;
        d.debug_name         = fallback_name.empty() ? String( "Material" ) : String( fallback_name );
        d.vertex_layout_name = VertexLayoutName;
        d.flags              = Flags;
        return d;
    }

    NSTRUCTV(
        MaterialComponent, NC_F( MaterialComponent, Source ), NC_F( MaterialComponent, Instance ),
        NC_F( MaterialComponent, Textures ), NC_F( MaterialComponent, TextureCount ),
        NC_F( MaterialComponent, DrawMode )
    )
};

} // namespace nc
