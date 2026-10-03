project(ncore_editor LANGUAGES CXX)

add_library(ncore_editor SHARED)

target_sources(
  ncore_editor
  PRIVATE "include/editor/ncore_editor.h"
		  "src/editor_camera.cpp"
		  "src/editor_camera.h"
          "src/editor_state.h"
          "src/gui_plugin.h"
          "src/gui_plugin.cpp"
		  "src/imgui_style.h"
          "src/imgui_utils.h"
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
            -Wno-format-nonliteral>)

# Reflection code generation — registers editor component types
# (EditorCameraTag, GuiStateComponent, EditorState) for find<T>().
file(GLOB_RECURSE NCORE_EDITOR_HEADERS
     LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

nc_add_reflection_gen(ncore_editor HEADERS ${NCORE_EDITOR_HEADERS}
                      FLAGS_FROM "tools/editor/src/")

# .gen.h files re-include their source header by relative path; headers under
# src/ resolve as bare <editor_state.h>.
target_include_directories(ncore_editor PRIVATE
                           "${CMAKE_CURRENT_SOURCE_DIR}/src")
