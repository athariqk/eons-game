#pragma once

#include <memory>

#include <ncore/kernel/collection.h>
#include <ncore/kernel/rid.h>
#include <ncore/kernel/structures.h>
#include <ncore/modules/module.h>
#include <ncore/modules/video/render_device.h>
#include <ncore/modules/video/render_surface.h>

namespace nc {

class Image;
class Material;
class Mesh;

class NCORE_API GraphicsModule : public IModule {
    NCLASS( GraphicsModule, IModule )

public:
    struct NCORE_API RenderSettings {
        bool VSync = true;
        NSTRUCT( RenderSettings, NC_F( RenderSettings, VSync ) )
    };

    const RenderSettings& get_settings() const
    {
        return settings;
    }

    Error init( ConfFile& cfg_file ) override;
    void finalize() override;

    RID create_surface( void* native_whnd, Vec2 surface_size );
    void destroy_surface( RID surface_rid );
    IRenderSurface* get_surface( RID surface_rid ) const;

    void begin_frame( RID surface_rid );
    void end_frame( RID surface_rid );
    void set_clear_color( RID surface_rid, Color color );
    void set_vsync( RID surface_rid, bool enabled );
    void set_render_size( RID surface_rid, Vec2 size );
    Vec2 get_surface_size( RID surface_rid ) const;

    RID upload_image( const Image& image );
    RID upload_pipeline( const Material& material );
    RID upload_mesh( const Mesh& mesh );
    void destroy_resource( RID rid );

    void fill_rect( Vec4 rect, Color color );
    void draw_rect( Vec4 rect, Color color, float thickness = 1.0f );
    void draw_line( Vec2 from, Vec2 to, Color color, float thickness = 1.0f );
    void draw_point( Vec2 pos, Color color );
    void draw_textured_quad( RID texture, Vec4 dest, Vec4 src, Color tint );

    void draw_circle( float X, float Y, int radius, Color color, bool filled, bool edge );
    void draw_convex_polygon_filled( const Vec2* vertices, int count, const Color& color );

    void draw_indexed(
        const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count, RID texture,
        Vec4 clip_rect
    );

    void* get_native_device() const;
    void* get_native_handle( RID rid ) const;

private:
    RenderSettings settings;
    std::unique_ptr<IRenderDevice> device;
    UnorderedMap<RID, std::unique_ptr<IRenderSurface>> surfaces;
    RID white_texture;
};

} // namespace nc
