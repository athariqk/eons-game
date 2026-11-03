#include <editor_components.h>

#include <ncore/runtime/components.h>
#include <ncore/runtime/ecs/ecs_query.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/video/render_service.h>

#include <panels/viewport_panel.h>

namespace nc::editor {

namespace {

EcsEntity resolve_camera_entity( EcsWorld& world, ViewportPanel::CameraSource source )
{
    EcsEntity result = INVALID_ENTITY_ID;

    if (source == ViewportPanel::CameraSource::EditorCamera) {
        auto query = world.query( "EditorViewport_EditorCamera" ).with<EditorCameraTag, CameraComponent>().build();
        for (auto ctx : query.entities())
            result = ctx.entity();
    } else {
        auto query = world.query( "EditorViewport_ActiveCamera" ).with<ActiveCameraTag, CameraComponent>().build();
        for (auto ctx : query.entities())
            result = ctx.entity();
    }

    return result;
}

} // namespace

ViewportPanel::ViewportPanel(
    const String& p_title, DockSlot p_slot, int p_order, CameraSource p_source, bool p_preserve_aspect
) : EditorPanel( p_title, p_slot, p_order, true, true ), source( p_source ), preserve_aspect( p_preserve_aspect )
{}

ViewportPanel::~ViewportPanel()
{
    // The renderer outlives the editor: targets are released explicitly from
    // Editor::release_gpu() before the GUI plugin tears down.
    color_rt = 0;
    depth_rt = 0;
}

void ViewportPanel::add_overlay( Ptr<ViewportOverlay> overlay )
{
    overlays.push_back( std::move( overlay ) );
}

void ViewportPanel::release_targets( RenderService& renderer )
{
    if (color_rt)
        renderer.destroy_rid( color_rt );
    if (depth_rt)
        renderer.destroy_rid( depth_rt );

    color_rt    = 0;
    depth_rt    = 0;
    target_size = Vec2f();
}

void ViewportPanel::release_gpu( RenderService& renderer )
{
    release_targets( renderer );
}

void ViewportPanel::update_target( EditorFrame& ctx, ImVec2 size, RenderService& renderer )
{
    const int width  = static_cast<int>( size.x );
    const int height = static_cast<int>( size.y );
    if (width < 1 || height < 1)
        return;

    if (width == static_cast<int>( target_size.x ) && height == static_cast<int>( target_size.y ))
        return;

    release_targets( renderer );

    const Vec2i target( width, height );
    color_rt    = renderer.texture_render_create( target, color_format );
    depth_rt    = renderer.texture_render_create( target, depth_format );
    target_size = Vec2f( size.x, size.y );
}

void ViewportPanel::bind_camera( EditorFrame& p_ctx, bool open )
{
    auto& ecs_world = p_ctx.scene->get_ecs();

    camera_entity = resolve_camera_entity( ecs_world, source );

    auto* cam_cmp = static_cast<CameraComponent*>(
        ecs_world.get_component_ptr( camera_entity, TypeDB::find<CameraComponent>() )
    );
    if (!cam_cmp) {
        camera = 0;
        return;
    }

    camera = cam_cmp->source;

    cam_cmp->render_to_screen = false;
    cam_cmp->draw_canvas      = false;

    if (open && color_rt) {
        cam_cmp->render_texture = color_rt;
        cam_cmp->depth_texture  = depth_rt;
        cam_cmp->display_rect   = Rect2i( 0, 0, static_cast<int>( target_size.x ), static_cast<int>( target_size.y ) );
    } else {
        // clearing the target skips the camera's render pass entirely.
        cam_cmp->render_texture = 0;
        cam_cmp->depth_texture  = 0;
    }
}

void ViewportPanel::draw_contents( EditorFrame& p_ctx )
{
    auto renderer = p_ctx.scene->get_app_ctx()->services.resolve<RenderService>();
    auto time     = p_ctx.scene->get_ecs().get_singleton<TimeComponent>();
    if (!renderer || !time)
        return;

    is_hovered = ImGui::IsWindowHovered();
    is_focused = ImGui::IsWindowFocused( ImGuiFocusedFlags_RootAndChildWindows );

    screen_pos = ImGui::GetCursorScreenPos();
    content    = ImGui::GetContentRegionAvail();

    update_target( p_ctx, content, *renderer );

    bind_camera( p_ctx, true );

    bool drew_image = false;

    if (color_rt && content.x > 0.0f && content.y > 0.0f) {
        ImVec2 draw_size = content;

        if (preserve_aspect && target_size.y > 0.0f) {
            const float rt_aspect    = target_size.x / target_size.y;
            const float avail_aspect = content.x / content.y;
            if (avail_aspect > rt_aspect)
                draw_size.x = content.y * rt_aspect;
            else
                draw_size.y = content.x / rt_aspect;

            ImGui::SetCursorPos( ImVec2(
                ImGui::GetCursorPosX() + ( content.x - draw_size.x ) * 0.5f,
                ImGui::GetCursorPosY() + ( content.y - draw_size.y ) * 0.5f
            ) );
        }

        ImGui::Image( reinterpret_cast<ImTextureID>( static_cast<uintptr_t>( color_rt.value ) ), draw_size );

        ViewportFrame frame;
        frame.renderer     = renderer;
        frame.time         = time;
        frame.camera       = camera;
        frame.screen_pos   = screen_pos;
        frame.content_size = draw_size;
        frame.draw_list    = ImGui::GetWindowDrawList();

        for (auto& overlay : overlays) {
            if (overlay)
                overlay->draw( frame );
        }

        drew_image = true;
    }

    p_ctx.any_viewport_hovered = p_ctx.any_viewport_hovered || ( drew_image && is_hovered );
    p_ctx.any_viewport_focused = p_ctx.any_viewport_focused || is_focused;
}

ImGuiWindowFlags ViewportPanel::window_flags() const
{
    return ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
}

void ViewportPanel::pre_begin( EditorFrame& )
{
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 0.0f );
}

void ViewportPanel::post_end( EditorFrame& )
{
    ImGui::PopStyleVar( 2 );
}

void ViewportPanel::on_collapsed( EditorFrame& p_ctx )
{
    is_hovered = false;
    is_focused = false;

    // Keep the camera from rendering to screen while the viewport is closed.
    bind_camera( p_ctx, false );
}

//------------------------------------------------------------------------------
// Overlays
//------------------------------------------------------------------------------

GizmoOverlay::GizmoOverlay( Scene& p_scene, EditorSelection& p_selection, EditorSettings& p_settings ) :
    scene( p_scene ), selection( p_selection ), settings( p_settings )
{}

void GizmoOverlay::draw( ViewportFrame& p_frame )
{
    if (!p_frame.renderer || !p_frame.camera)
        return;

    Node* selected = scene.get_node_from_entity( selection.get_selected_eid() );
    if (!selected || !selected->has_component<Transform3DComponent>())
        return;

    auto* xform = selected->get_component<Transform3DComponent>();

    ImGuizmo::SetDrawlist( p_frame.draw_list );
    ImGuizmo::SetRect( p_frame.screen_pos.x, p_frame.screen_pos.y, p_frame.content_size.x, p_frame.content_size.y );

    auto* cam_attribs   = p_frame.renderer->camera_attribs_get( p_frame.camera );
    auto projection_mat = p_frame.renderer->camera_perspective_get( p_frame.camera );
    auto view_mat       = cam_attribs->transform.affine_inverse();

    auto* parent       = selected->get_parent();
    auto* parent_xform = parent ? parent->get_component<Transform3DComponent>() : nullptr;
    Mat4 parent_global = parent_xform ? parent_xform->global : Mat4::identity();
    Mat4 contrib       = xform->parent_contrib( parent_global );

    Mat4 world     = xform->global;
    auto mode      = settings.global_xform_gizmo ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
    auto operation = static_cast<ImGuizmo::OPERATION>( settings.xform_gizmo_operation );

    if (ImGuizmo::Manipulate( view_mat.data(), projection_mat.data(), operation, mode, world.data() ))
        xform->from_matrix( contrib.affine_inverse() * world );
}

void StatsOverlay::draw( ViewportFrame& p_frame )
{
    if (!p_frame.renderer || !p_frame.time)
        return;

    const auto* time  = p_frame.time;
    const ImVec2 pos  = p_frame.screen_pos;
    const ImVec2 size = p_frame.content_size;

    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );

    constexpr ImGuiWindowFlags FLAGS = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                       ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground;

    constexpr float PAD = 10.0f;
    ImGui::SetNextWindowPos( ImVec2( pos.x + size.x - PAD, pos.y + PAD ), ImGuiCond_Always, ImVec2( 1.0f, 0.0f ) );
    ImGui::SetNextWindowSize( ImVec2( 200, 0 ) );

    if (ImGui::Begin( "##viewport_stats", nullptr, FLAGS )) {
        ImGui::Text( "Ticks: %u", time->ticks );
        ImGui::Text( "FPS: %.3f", time->fps );
        ImGui::Text( "Frame count: %d", time->frame_count );

        const auto gfx_stats = p_frame.renderer->get_graphics_api()->get_stats();
        ImGui::Text( "GPU time: %.3f ms", gfx_stats.gpu_duration_ms );
        if (ImGui::TreeNode( "Timestamps (ms)" )) {
            auto* gfx = p_frame.renderer->get_graphics_api();
            for (uint32_t i = 0; i < GfxInterface::MAX_TIMESTAMP_SLOTS; ++i)
                ImGui::Text( "  %s: %.3f ms", gfx->get_timestamp_name( i ), gfx_stats.timestamps_ms[i] );

            ImGui::TreePop();
        }

        ImGui::Text( "IA Prims: %llu", gfx_stats.input_primitives );
        ImGui::Text( "IA Verts: %llu", gfx_stats.input_vertices );
        ImGui::Text( "VS Invokes: %llu", gfx_stats.vs_invocations );
        ImGui::Text( "PS Invokes: %llu", gfx_stats.ps_invocations );
        ImGui::Text( "Clipping Prims: %llu", gfx_stats.clipping_primitives );
        ImGui::Text( "Clipping Invokes: %llu", gfx_stats.clipping_invocations );
    }
    ImGui::End();

    ImGui::PopStyleVar();
}

} // namespace nc::editor
