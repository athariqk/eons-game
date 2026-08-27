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
