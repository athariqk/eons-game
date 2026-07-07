#pragma once

#include <memory>

#include <ncore/kernel/rid.h>
#include <ncore/kernel/structures.h>
#include <ncore/modules/module.h>
#include <ncore/modules/video/render_backend.h>

namespace nc {

class Image;
class Material;
class Mesh;

class GraphicsModule : public IModule {
    NCLASS( GraphicsModule, IModule )

public:
    explicit GraphicsModule( std::unique_ptr<IRenderBackend> backend );

    Error init() override;
    void finalize() override;

    void begin_frame();
    void end_frame();
    void set_clear_color( Color color );
    void set_vsync( bool enabled );
    void set_render_size( Vec2 size );
    Vec2 get_surface_size() const;

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
    RID create_white_texture_();

    std::unique_ptr<IRenderBackend> renderer;
    RID white_texture;
};

} // namespace nc
