#include "editor_factory.h"

#include <ncore/app_context.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/asset_service.h>
#include <ncore/services/video/render_service.h>
#include <ncore/structs/quaternion.h>
#include <ncore/structs/vector.h>

#include "editor.h"
#include "inspector/drawers/color_drawer.h"
#include "inspector/drawers/math_drawer.h"
#include "inspector/drawers/resource_drawer.h"
#include "inspector/plugins/material_inspector.h"

#include <editor/panels/assets_panel.h>
#include <editor/panels/console_panel.h>
#include <editor/panels/inspector_panel.h>
#include <editor/panels/scene_tree_panel.h>
#include <editor/panels/toolbar_panel.h>

namespace sea::editor {

namespace {

void register_default_panels( Editor& editor, nc::RenderService& renderer )
{
    auto& scene = editor.get_scene();

    editor.register_panel( std::make_unique<ToolbarPanel>( scene, editor.get_settings() ) );

    editor.register_panel( std::make_unique<SceneTreePanel>( scene, editor.get_selection(), editor.get_assets() ) );
    auto inspector_panel = std::make_unique<InspectorPanel>(
        scene, editor.get_selection(), editor.get_assets(), editor.get_property_drawers()
    );
    inspector_panel->add_plugin( std::make_unique<MaterialInspectorPlugin>( editor.get_assets() ) );
    editor.register_panel( std::move( inspector_panel ) );
    editor.register_panel( std::make_unique<AssetsPanel>( editor.get_assets(), editor.get_selection() ) );
    editor.register_panel( std::make_unique<ConsolePanel>( editor.get_logs(), true ) );

    editor.register_drawer( nc::TypeDB::find<nc::Vec3f>(), draw_vec3f_property );
    // editor.register_drawer( nc::TypeDB::find<nc::Vec3i>(), draw_vec3i_property );
    editor.register_drawer( nc::TypeDB::find<nc::Quaternion>(), draw_quaternion_property );
    editor.register_drawer( nc::TypeDB::find<nc::Rect2f>(), draw_rect2f_property );
    editor.register_drawer( nc::TypeDB::find<nc::Rect2i>(), draw_rect2i_property );
    editor.register_drawer( nc::TypeDB::find<nc::Mat3>(), draw_matrix3f_property );
    editor.register_drawer( nc::TypeDB::find<nc::Mat4>(), draw_matrix4f_property );
    editor.register_drawer( nc::TypeDB::find<nc::Color>(), draw_color_property );
    editor.register_drawer( nc::TypeDB::find<nc::RID>(), draw_rid_property );
}

} // namespace

nc::Ptr<Editor> create_editor( nc::Scene& p_scene )
{
    auto* app = p_scene.get_app_ctx();
    if (!app)
        return nullptr;

    auto* renderer = app->services.resolve<nc::RenderService>();
    if (!renderer)
        return nullptr;

    // AssetService is optional: the editor still browses compiled assets without it.
    auto* compiler = app->services.resolve<nc::AssetService>();

    auto editor = std::make_unique<Editor>( p_scene, compiler );
    editor->get_settings().load( Editor::DEFAULT_SETTINGS_PATH );

    register_default_panels( *editor, *renderer );

    editor->apply_settings();
    editor->sort_panels();
    return editor;
}

} // namespace sea::editor
