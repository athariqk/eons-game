#pragma once

#include <memory>

#include <ncore/core/collection.h>
#include <ncore/core/rid.h>
#include <ncore/core/structures.h>
#include <ncore/modules/module.h>
#include <ncore/modules/video/renderer/geometry.h>

#include "renderer/renderer_context.h"
#include "renderer/renderer_storage.h"
#include "renderer/world_renderer.h"
#include "rhi.h"

namespace nc {

class Image;
class Shader;
class MaterialTemplate;
class Mesh;

class NCAPI RenderModule : public IModule {
    NCLASS( RenderModule, IModule )

public:
    struct NCAPI RenderSettings {
        bool VSync = true;
        NSTRUCT( RenderSettings, NC_F( RenderSettings, VSync ) )
    };

    const RenderSettings& get_settings() const
    {
        return settings;
    }

    Error init( ConfFile& cfg_file ) override;
    void shutdown() override;

    RID swapchain_create( void* whnd, Vec2 size );
    void swapchain_set_size( RID sc, Vec2 size );
    Vec2 swapchain_get_size( RID sc );
    void swapchain_destroy( RID sc );

    RID material_create( const MaterialTemplate& tmpl );
    void material_set_texture( RID material, RID texture, uint32_t slot );

    RID create_mesh( const Mesh& mesh );
    RID create_texture_2d( const Image& image );

    void destroy_rid( RID rid );

    void frame_begin();
    void frame_end();

    void canvas_draw_triangles(
        std::span<const Vertex2D> verts, std::span<const uint16_t> indices, RID material, Vec4 clip = {}
    );
    void canvas_draw_quad( Vec2 pos, Vec2 size, RID material, Vec4 uv_rect, Color tint, Vec4 clip = {} );

    /**
     * @brief Return the internal RHI and storage for advanced use.
     */
    RendererContext* get_context()
    {
        return &ctx;
    }

private:
    void ensure_canvas_vb_( uint32_t needed );
    void ensure_canvas_ib_( uint32_t needed );

    RenderSettings settings;
    RendererContext ctx;
    // WorldRenderer m_world;
    Vector<RID> swapchains;

    // Canvas
    float ortho_proj[16] = {};
    RID canvas_vb;
    RID canvas_ib;
    uint32_t canvas_vb_size = 0;
    uint32_t canvas_ib_size = 0;
};

} // namespace nc
