add_executable(seaengine)

target_sources(
  seaengine
  PRIVATE "main.cpp"
		  # Core editor
		  "src/editor/editor.h"
		  "src/editor/editor.cpp"
		  "src/editor/editor_factory.h"
		  "src/editor/editor_factory.cpp"
		  "src/editor/editor_components.h"
		  "src/editor/editor_components.cpp"
		  "src/editor/editor_selection.h"
		  "src/editor/editor_selection.cpp"
		  "src/editor/editor_frame.h"
		  "src/editor/editor_bootstrap.h"
		  "src/editor/editor_bootstrap.cpp"
		  
		  # Panels
		  "src/editor/panels/editor_panel.h"
		  "src/editor/panels/editor_panel.cpp"
		  "src/editor/panels/toolbar_panel.h"
		  "src/editor/panels/toolbar_panel.cpp"
		  "src/editor/panels/scene_tree_panel.h"
		  "src/editor/panels/scene_tree_panel.cpp"
		  "src/editor/panels/inspector_panel.h"
		  "src/editor/panels/inspector_panel.cpp"
		  "src/editor/panels/assets_panel.h"
		  "src/editor/panels/assets_panel.cpp"
		  "src/editor/panels/console_panel.h"
		  "src/editor/panels/console_panel.cpp"
		  
		  # Inspector
		  "src/editor/inspector/inspector_plugin.h"
		  "src/editor/inspector/property_draw_context.h"
		  "src/editor/inspector/property_drawer_db.h"
		  "src/editor/inspector/property_drawer_db.cpp"
		  "src/editor/inspector/property_grid.h"
		  "src/editor/inspector/property_grid.cpp"
		  "src/editor/inspector/utils.h"
		  
		  "src/editor/inspector/drawers/color_drawer.h"
		  "src/editor/inspector/drawers/color_drawer.cpp"
		  "src/editor/inspector/drawers/math_drawer.h"
		  "src/editor/inspector/drawers/math_drawer.cpp"
		  "src/editor/inspector/drawers/resource_drawer.h"
		  "src/editor/inspector/drawers/resource_drawer.cpp"
		  
		  "src/editor/inspector/plugins/material_inspector.h"
		  "src/editor/inspector/plugins/material_inspector.cpp"
		  
		  "src/editor/inspector/widgets/scalar_widgets.h"
		  "src/editor/inspector/widgets/scalar_widgets.cpp"
		  "src/editor/inspector/widgets/vector_widgets.h"
		  "src/editor/inspector/widgets/vector_widgets.cpp"
		  
		  # IO / Assets
		  "src/editor/io/asset_paths.h"
		  "src/editor/io/asset_paths.cpp"
		  "src/editor/io/editor_assets.h"
		  "src/editor/io/editor_assets.cpp"
		  "src/editor/io/asset_importer.h"
		  "src/editor/io/asset_importer.cpp"
		  "src/editor/io/thumbnail_cache.h"
		  "src/editor/io/thumbnail_cache.cpp"
		  
		  # Plugins
		  "src/editor/plugins/gui_plugin.h"
		  "src/editor/plugins/gui_plugin.cpp"
		  
		  # Filesystem helpers
		  "src/editor/fs/utils.h"
		  "src/editor/fs/utils.cpp"
		  
		  # Backends
		  "src/editor/backends/imgui/imgui_style.h"
		  "src/editor/backends/imgui/imgui_utils.h"
          "src/tests/test_compute_shader.h"
          "src/tests/test_compute_shader.cpp"
          "src/water/water_sim.h"
          "src/water/water_sim.cpp"
          "src/water/water_mesh.h"
          "src/water/water_mesh.cpp"
          "src/water/wave_cascade_params.h"
          "src/water/wave_generator.h"
          "src/water/wave_generator.cpp"
          "src/main_scene.h"
          "src/main_scene.cpp")

target_link_libraries(seaengine PRIVATE ncore)

add_subdirectory(external)

set_target_properties(
  seaengine
  PROPERTIES
    DEBUG_POSTFIX "_d"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:seaengine>"
    VS_DEBUGGER_ENVIRONMENT
    "ASAN_OPTIONS=fast_unwind_on_malloc=0:malloc_context_size=20:halt_on_error=0;ASAN_SYMBOLIZER_PATH=C:/tools/LLVM/bin/llvm-symbolizer.exe")

target_compile_definitions(seaengine PRIVATE $<$<CONFIG:Debug>:DEBUG=1>
                                             $<$<CONFIG:Dist>:DIST=1>)

# ASAN runtime DLL must sit next to the exe (Debug + clang-cl only).
# (The ASAN copy now applies to the unified `seaengine` target for both
# Explore Mode and Editor mode via `--editor`.)
if(SEA_TRACK_ASAN
   AND CMAKE_CXX_COMPILER_ID MATCHES "Clang"
   AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
  set(_asan_runtime
      "${LLVM_INSTALL_PREFIX}/lib/windows/clang_rt.asan_dynamic-x86_64.dll")
  if(EXISTS "${_asan_runtime}")
    add_custom_command(
      TARGET seaengine
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_asan_runtime}"
              "$<TARGET_FILE_DIR:seaengine>"
      COMMENT "Copying ASAN runtime DLL")
  else()
    message(
      WARNING
        "ASAN runtime DLL not found at ${_asan_runtime} — Debug builds will fail to start"
    )
  endif()
endif()

# Copy the (single unified) config file (used for both Explore Mode (default) and --editor).
add_custom_command(
  TARGET seaengine
  POST_BUILD
  COMMAND
    "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_SOURCE_DIR}/seaengine/seaengine.ini"
    "$<TARGET_FILE_DIR:seaengine>/seaengine.ini"
  COMMENT "Copying seaengine.ini"
  VERBATIM)

# Reflection code generation — covers both the core seaengine sources and the
# editor sources (editor code is always compiled in as plain sources).
file(
  GLOB_RECURSE SEA_ENGINE_HEADERS
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/editor/*.h")

generate_headers_reflection(seaengine HEADERS ${SEA_ENGINE_HEADERS} FLAGS_FROM
                      "seaengine/")

target_include_directories(seaengine
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src")
