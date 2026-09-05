#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>
#include <ncore/resources/material_shader.h>
#include <ncore/services/video/rhi_types.h>

namespace nc {

class MaterialTemplate;

constexpr int MAXIMUM_MATERIAL_TEXTURE = 8;

/**
 * Scene instance of a material (Esoterica Material + runtime GPU RIDs).
 *
 * Source  -> MaterialTemplate resource (shader + default params)
 * Instance -> GPU material RID from RenderService::material_create
 * Textures / Params / Flags -> per-entity overrides
 */
struct NCAPI MaterialComponent {
    RID Source   = 0;
    RID Instance = 0;

    Array<RID, MAXIMUM_MATERIAL_TEXTURE> Textures = {};
    int TextureCount = 0;

    /** Rare geometry override (debug wireframe). Prefer flags for two-sided. */
    rhi::FillMode DrawMode = rhi::FillMode::SOLID;

    /** Per-instance surface flags; None = use template/shader. */
    MaterialShaderFlags Flags = MaterialShaderFlags::None;

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

    MaterialDrawBucket draw_bucket() const
    {
        return draw_bucket_from_flags( Flags );
    }

    NSTRUCTV(
        MaterialComponent, NC_F( MaterialComponent, Source ), NC_F( MaterialComponent, Instance ),
        NC_F( MaterialComponent, Textures ), NC_F( MaterialComponent, TextureCount ),
        NC_F( MaterialComponent, DrawMode )
    )
};

} // namespace nc
