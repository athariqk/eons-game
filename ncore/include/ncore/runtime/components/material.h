#pragma once

#include <cstdint>

#include <ncore/core/collection.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/runtime/resources/resource_handle.h>

namespace nc {

NC_COMPONENT_API( MaterialComponent )
{
    static constexpr int MAX_TEXTURE = 12;

    REFLECT RID shader                = 0;
    REFLECT int texture_count         = 0;
    REFLECT RID textures[MAX_TEXTURE] = {};
    REFLECT gfx::FillMode draw_mode   = gfx::FillMode::SOLID;

    struct Param {
        String name;
        DynamicArray<uint8_t> data;
    };
    DynamicArray<Param> params;

    /**
     * @brief Add a GPU texture to this material.
     *
     * Obtain GPU textures from RenderService::texture_*_create() methods.
     */
    void add_texture( RID p_texture_rid )
    {
        NC_FAIL_MSG_RET( texture_count < MAX_TEXTURE, "Max texture count reached." );
        textures[texture_count++] = p_texture_rid;
    }

    /**
     * @brief Set a named material uniform (upsert).
     *
     * NOTE: expensive operation.
     */
    void set_param( StringView p_name, const void* p_data, size_t p_size )
    {
        for (auto& p : params) {
            if (p.name == p_name) {
                p.data.assign( static_cast<const uint8_t*>( p_data ), static_cast<const uint8_t*>( p_data ) + p_size );
                return;
            }
        }

        Param entry;
        entry.name = String( p_name );
        entry.data.assign( static_cast<const uint8_t*>( p_data ), static_cast<const uint8_t*>( p_data ) + p_size );
        params.push_back( std::move( entry ) );
    }

    /**
     * @brief Set a named material uniform from a typed value.
     *
     * NOTE: expensive operation.
     */
    template<typename T>
    void set_params( StringView p_name, const T& p_data )
    {
        set_param( p_name, &p_data, sizeof( T ) );
    }
};

NC_COMPONENT_API( MaterialRenderComponent )
{
    /**
     * @brief GPU material RID.
     */
    REFLECT_HINT( rtti::PropertyFlags::HIDDEN ) RID handle = 0;
};

} // namespace nc
