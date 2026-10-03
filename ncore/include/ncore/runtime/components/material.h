#pragma once

#include <cstdint>

#include <ncore/core/collection.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/runtime/resources/resource_handle.h>

namespace nc {

NC_COMPONENT_API( MaterialComponent )
{
    static constexpr int MAXIMUM_MATERIAL_TEXTURE = 12;

    REFLECT Res<MaterialShader> Shader;
    REFLECT Array<RID, MAXIMUM_MATERIAL_TEXTURE> Textures = {};
    REFLECT int TextureCount                              = 0;
    REFLECT gfx::FillMode DrawMode                        = gfx::FillMode::SOLID;

    /**
     * @brief A single named material uniform (code-only, not reflected).
     *
     * The name is resolved against the shader's reflected fields when the GPU
     * material is created; the bytes are uploaded verbatim to that field's offset.
     */
    struct Param {
        String name;
        DynamicArray<uint8_t> data;
    };
    DynamicArray<Param> Params; // set via set_param / set_params

    /**
     * @brief Add a GPU texture to this material.
     *
     * Obtain GPU textures from RenderService::texture_*_create() methods.
     */
    void add_texture( RID texture_rid )
    {
        NC_FAIL_MSG_RET( TextureCount < MAXIMUM_MATERIAL_TEXTURE, "Max texture count reached." );
        Textures[TextureCount++] = texture_rid;
    }

    /**
     * @brief Set a named material uniform (upsert).
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
