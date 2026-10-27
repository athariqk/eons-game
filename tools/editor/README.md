# ncore_editor

Editor tooling for the engine, built as a shared library (`ncore_editor_d.dll`,
Debug-only) that a game links and enables from its own `Scene`.

## Layout

```
include/editor/       Public entry points (register/unregister the editor)
src/editor.h/.cpp     Editor shell: EditorContext, EditorPanel, EditorPlugin /
                      EditorInterface, EditorSettings, Selection, LogStore
src/editor_components.h   ECS components owned by the editor
                      (EditorCameraTag, SelectedTag)
src/ncore_editor.cpp  Editor instance, plugin list, the four ECS systems and
                      the editor free camera
src/panels/           One file per window:
                        assets_panel      asset browser + RID picker widget
                        inspector_panel   RTTI property sheet + handler dispatch
                        material_inspector  material handler (uniforms, textures)
                        logs_panel        console view over Editor's LogStore
                        scene_tree_panel  hierarchy, context menu, drag-drop
                        toolbar_panel     gizmo mode / wireframe / plugin buttons
                        viewport_panel    offscreen viewport + overlays
src/plugins/          gui_plugin (ImGui backend) + editor_plugins.h
                      (header-only built-in plugins)
src/io/               Asset index, importer, resource-compile helpers
src/backends/imgui/   ImGui style + helpers
```

## Plugin model

The engine only knows four systems (`EditorDockspace`, `EditorUi`,
`EditorUiCapture`, `EditorCanvasPass`). Everything else is a plugin:

```cpp
class EditorPlugin {
public:
    virtual StringView name() const = 0;
    virtual void enter( EditorInterface& ed ) = 0;
    virtual void exit( EditorInterface& ed ) { }
};
```

A plugin composes the editor through verbs and never touches shell internals:

```cpp
auto* view = static_cast<ViewportPanel*>(
    ed.add_panel( DockSlot::Center, 0, std::make_unique<ViewportPanel>( ... ) ) );
view->add_overlay( std::make_unique<GizmoOverlay>() );

ed.add_inspector_handler( std::make_unique<MaterialInspectorHandler>() );
ed.add_property_widget( rtti::TypeRegistry::find<RID>(), widget_fn );

ed.add_command( "View", "Something", []{ /* ... */ } ); // "" = toolbar button
```

Panels, commands, inspector handlers and property widgets are tagged with the
plugin index; `~Editor()` unwinds the plugins in reverse order. The shell never
knows a concrete panel type.

## Docking

Panels declare a `DockSlot` (Godot-style) plus a tab order at registration:

| Slot | Contents |
| --- | --- |
| `Top` | Toolbar |
| `LeftUpper` | Scene Tree |
| `LeftLower` | Assets |
| `RightUpper` | Inspector |
| `Bottom` | Game View, Logs (tabs) |
| `Center` | Scene View |
| `Floating` | free windows |

`Editor::build_default_layout()` splits the dockspace into those slots once and
docks every panel by slot in `(slot, order)` order; panels are sorted the same
way for drawing, so tab order and draw order cannot disagree.

## Editor state

- **Selection** — `SelectedTag` in the ECS (so engine-side systems can read it)
  with `Selection` as the editor-facing API and a cached id in `EditorContext`.
- **Logs** — `LogStore` on `Editor`: lines are staged by any thread, drained on
  the render thread in `Editor::begin_frame()` and rendered by `LogsPanel` as a
  pure view. Ingestion keeps running while the panel is hidden.
- **Settings** — `EditorSettings` stores gizmo mode, wireframe and per-panel
  visibility in `editor.ini`; `imgui.ini` keeps ImGui's own window state.
- **Context** — `EditorContext` is refreshed once per frame and carries scene,
  world, services, selection, log store, the command list and the inspector
  extension registries.

## Scene mutations

Systems (including the editor UI) run while Flecs has the world read-only, so
structural changes from input must be queued and executed outside
`ecs_world.progress()`:

```cpp
scene->queue_task( [scene, parent_id]() {                 // create
    if ( auto* parent = scene->node_from_entity( parent_id ) )
        parent->create_child();
} );

scene->queue_destroy_node( node );                        // delete
```

`Scene::on_variable_update()` drains queued tasks, then pending deletions,
before ticking ECS. Creating entities inline from a system hands you deferred
(temporary) ids that poison pairs, queries and deletion.

Unnamed nodes get a generated display name (`Node N`) instead of an anonymous
ECS entity, because anonymous creation through the builder returns internal ids
on this Flecs version.

## Inspector extension points

The inspector is composed, not hardcoded:

- **InspectorHandler** — per-component extension, the Godot `EditorInspectorPlugin`
  equivalent. `can_handle(type, component)` + `draw(ctx, node, type, component)`;
  handlers run after the generic sheet, and `replaces_default()` lets one take
  the sheet over entirely. The material section (shader uniforms, texture count,
  apply-to-all) is the first handler (`panels/material_inspector.*`).
- **PropertyWidget** — per-reflected-type value widget, registered with
  `add_property_widget(type, fn)`. The assets plugin registers the RID widget, so
  the inspector does not know about the asset picker. A widget that declines
  (returns false) falls back to the default row.

## Panels and viewports

A panel is one object: `void draw( EditorContext& )`. Panels are drawn every
frame, even when hidden, so background work keeps running.

`ViewportPanel` owns its render targets, resolves its camera (editor camera or
the active game camera), binds `CameraComponent::RenderTexture` and publishes
hover/focus into the shared context. Scene View and Game View are two instances;
per-viewport extras are `ViewportOverlay` objects (gizmo, stats) owned by the
viewport that the plugin creates.

The scene tree sorts siblings by entity id before drawing: Flecs stores entities
per archetype, so adding a tag or component moves a row in child-query order and
would otherwise make the selected node jump to the bottom.

## Frame order

```
PRE_UPDATE  -1000  ImGui::NewFrame (gui_plugin)
PRE_UPDATE   -950  DockSpaceOverViewport + first-run layout builder
PRE_UPDATE   -900  ImGuizmo::BeginFrame
UPDATE        -30  Editor::begin_frame + Editor::draw (menu bar, panels)
POST_UPDATE        ImGui::Render (gui_plugin), then UI-capture publish
POST_FRAME    ~15  swapchain canvas pass
```

Keep editor `.order()` values small: `finalize_ordering` only chains systems that
carry an order, so mixing large magnitudes with unordered systems makes relative
order undefined.

## Conventions

- Register editor component types (`register_editor_component_types`) before any
  editor query exists — Flecs refuses to apply `EcsCanToggle` afterwards.
- Panels never draw outside `Editor::draw()` — a window submitted after
  `ImGui::Render()` trips `g.WithinFrameScope`.
- Every `ImGui::Begin()` path must reach `ImGui::End()`.
- Reuse engine services (loader, renderer, input) instead of editing `ncore/`.
- New editor-wide features arrive as plugins, not as new shell code.
- Naming target: member variables `snake_case` with no trailing underscore,
  parameters prefixed `p_`. Where a member would collide with an API verb
  (`has_component()`, `emit_event()`, ...) the member takes an `_impl` suffix.
  The rules live in `.clang-tidy` at the repo root and are applied incrementally.
