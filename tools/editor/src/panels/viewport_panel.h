#pragma once

#include <imgui.h>

#include <editor.h>
#include <ncore/core/rid.h>

namespace nc::editor {

/**
 * @brief Everything an overlay needs to draw on top of a viewport image.
 */
struct ViewportFrame {
    EditorContext* ctx      = nullptr;
    RenderService* renderer = nullptr;
    RID camera              = 0;
    ImVec2 screen_pos       = ImVec2( 0, 0 );
    ImVec2 content_size     = ImVec2( 0, 0 );
    ImDrawList* draw_list   = nullptr;
};

/**
 * @brief Optional per-viewport extra (gizmo, stats overlay, etc).
 */
class ViewportOverlay {
public:
    virtual ~ViewportOverlay()                = default;
    virtual void draw( ViewportFrame& frame ) = 0;
};

/**
 * @brief An offscreen render target shown as an image inside a dockable window.
 *
 * The Scene View and Game View are two instances of this panel; the only
 * differences are which camera they follow and whether they letterbox.
 */
class ViewportPanel : public EditorPanel {
public:
    enum class CameraSource : uint8_t {
        EditorCamera,
        ActiveGameCamera
    };

    ViewportPanel( const String& p_title, DockSlot p_slot, int p_order, CameraSource p_source, bool p_preserve_aspect );

    ~ViewportPanel() override;

    void draw( EditorContext& ctx ) override;

    void release_gpu( RenderService& renderer ) override;

    void add_overlay( Ptr<ViewportOverlay> overlay );

    bool get_hovered() const
    {
        return is_hovered;
    }

    bool get_focused() const
    {
        return is_focused;
    }

private:
    void update_target( EditorContext& ctx, ImVec2 size );
    void bind_camera( EditorContext& ctx );
    void release_targets( RenderService& renderer );

    CameraSource source = CameraSource::EditorCamera;

    RID color_rt = 0;
    RID depth_rt = 0;
    Vec2f target_size;
    gfx::TextureFormat color_format = gfx::TextureFormat::RGBA8_UNORM_SRGB;
    gfx::TextureFormat depth_format = gfx::TextureFormat::D32_FLOAT;

    EcsEntity camera_entity = INVALID_ENTITY_ID;
    RID camera              = 0;

    ImVec2 screen_pos = ImVec2( 0, 0 );
    ImVec2 content    = ImVec2( 0, 0 );

    bool is_open         = false;
    bool is_hovered      = false;
    bool is_focused      = false;
    bool preserve_aspect = true;

    DynamicArray<Ptr<ViewportOverlay>> overlays;
};

/**
 * @brief ImGuizmo transform widget for the selected node.
 */
class GizmoOverlay final : public ViewportOverlay {
public:
    void draw( ViewportFrame& frame ) override;
};

/**
 * @brief Frame timing / GPU statistics overlay drawn inside a viewport.
 */
class StatsOverlay final : public ViewportOverlay {
public:
    void draw( ViewportFrame& frame ) override;
};

} // namespace nc::editor
