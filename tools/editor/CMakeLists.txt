project(ncore_editor LANGUAGES CXX)

add_library(ncore_editor SHARED)

target_sources(
  ncore_editor
  PRIVATE "include/editor/ncore_editor.h"
          "src/backends/imgui/imgui_style.h"
          "src/backends/imgui/imgui_utils.h"
          "src/io/asset_importer.h"
          "src/io/asset_importer.cpp"
          "src/io/editor_assets.h"
          "src/io/editor_assets.cpp"
		  "src/panels/assets_panel.h"
		  "src/panels/assets_panel.cpp"
		  "src/panels/inspector_panel.h"
		  "src/panels/inspector_panel.cpp"
		  "src/panels/logs_panel.h"
		  "src/panels/logs_panel.cpp"
          "src/panels/material_inspector.h"
          "src/panels/material_inspector.cpp"

		  "src/panels/scene_tree_panel.h"
		  "src/panels/scene_tree_panel.cpp"
		  "src/panels/toolbar_panel.h"
		  "src/panels/toolbar_panel.cpp"
		  "src/panels/viewport_panel.h"
		  "src/panels/viewport_panel.cpp"
          "src/plugins/editor_plugins.h"
          "src/plugins/gui_plugin.h"
          "src/plugins/gui_plugin.cpp"
          "src/editor.h"
          "src/editor.cpp"
          "src/editor_components.h"
          "src/ncore_editor.cpp")

add_subdirectory(external)

target_link_libraries(ncore_editor PUBLIC ncore)

target_include_directories(ncore_editor
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(
  ncore_editor PROPERTIES DEBUG_POSTFIX "_d" VS_DEBUGGER_WORKING_DIRECTORY
                                             "$<TARGET_FILE_DIR:ncore_editor>")

target_compile_definitions(ncore_editor PRIVATE $<$<CONFIG:Debug>:DEBUG=1>
                                                $<$<CONFIG:Dist>:DIST=1>)

target_compile_options(
  ncore_editor
  PRIVATE $<$<CXX_COMPILER_ID:Clang>:
          -Wall
          -Wextra
          -Werror
          -Wconversion
          -Wdouble-promotion
          -Wuninitialized
          -Wno-unused-parameter
          -Wno-unused-function
          -Wno-sign-conversion
          -Wno-padded
          -Wno-global-constructors
          -Wno-unsafe-buffer-usage
          -Wno-switch-default
          -Wno-invalid-offsetof
          -Wno-switch-enum
          -Wno-nonportable-system-include-path
          -Wno-lifetime-safety-intra-tu-suggestions
          -Wno-lifetime-safety-invalidation
          -Wno-unique-object-duplication
          -Wno-format-nonliteral
                    -Wno-documentation>)

# Reflection code generation
file(
  GLOB_RECURSE NCORE_EDITOR_HEADERS
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

generate_headers_reflection(ncore_editor HEADERS ${NCORE_EDITOR_HEADERS} FLAGS_FROM
                      "tools/editor/src/")

# .gen.h files re-include their source header by relative path; headers under
# src/ resolve as bare <editor_state.h>.
target_include_directories(ncore_editor
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
