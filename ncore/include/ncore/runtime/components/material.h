#pragma once

#include <cstdint>

#include <ncore/core/collection.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/runtime/resources/resource_handle.h>

namespace nc {

NC_COMPONENT_API( MaterialComponent )
{
    static constexpr int kMaxTexture = 12;

    REFLECT RID Shader                = 0;
    REFLECT int TextureCount          = 0;
    REFLECT RID Textures[kMaxTexture] = {};
    REFLECT gfx::FillMode DrawMode    = gfx::FillMode::SOLID;

    struct Param {
        String name;
        DynamicArray<uint8_t> data;
    };
    DynamicArray<Param> Params;

    /**
     * @brief Add a GPU texture to this material.
     *
     * Obtain GPU textures from RenderService::texture_*_create() methods.
     */
    void add_texture( RID texture_rid )
    {
        NC_FAIL_MSG_RET( TextureCount < kMaxTexture, "Max texture count reached." );
        Textures[TextureCount++] = texture_rid;
    }

    /**
     * @brief Set a named material uniform (upsert).
     *
     * NOTE: expensive operation.
     */
    void set_param( StringView name, const void* data, size_t size )
    {
        for (auto& p : Params) {
            if (p.name == name) {
                p.data.assign( static_cast<const uint8_t*>( data ), static_cast<const uint8_t*>( data ) + size );
                return;
            }
        }

        Param entry;
        entry.name = String( name );
        entry.data.assign( static_cast<const uint8_t*>( data ), static_cast<const uint8_t*>( data ) + size );
        Params.push_back( std::move( entry ) );
    }

    /**
     * @brief Set a named material uniform from a typed value.
     *
     * NOTE: expensive operation.
     */
    template<typename T>
    void set_params( StringView name, const T& data )
    {
        set_param( name, &data, sizeof( T ) );
    }
};

NC_COMPONENT_API( MaterialRenderComponent )
{
    /**
     * @brief GPU material RID.
     */
    REFLECT RID Handle = 0;
};

} // namespace nc
