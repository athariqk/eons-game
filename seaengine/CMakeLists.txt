add_executable(SeaEngine)

target_sources(SeaEngine PRIVATE
	"main.cpp"
	"src/tests/test_compute_shader.h"
	"src/tests/test_compute_shader.cpp"
	"src/water/water_sim.h"
	"src/water/water_sim.cpp"
	"src/water/wave_cascade_params.h"
	"src/water/wave_generator.h"
	"src/water/wave_generator.cpp"
	"src/world_scene.h"
	"src/world_scene.cpp")

target_include_directories(SeaEngine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(SeaEngine PRIVATE ncore ncore_editor)

set_target_properties(
  SeaEngine PROPERTIES DEBUG_POSTFIX "_d" VS_DEBUGGER_WORKING_DIRECTORY
                                          "$<TARGET_FILE_DIR:SeaEngine>")

target_compile_definitions(SeaEngine PRIVATE $<$<CONFIG:Debug>:DEBUG=1>
                                             $<$<CONFIG:Dist>:DIST=1>)

add_custom_command(
  TARGET SeaEngine
  POST_BUILD
  COMMAND
    "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/SeaEngine/assets"
    "$<TARGET_FILE_DIR:SeaEngine>/assets"
  COMMAND
    "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_SOURCE_DIR}/SeaEngine/seaengine.ini"
    "$<TARGET_FILE_DIR:SeaEngine>/seaengine.ini"
  COMMENT "Copying assets and config file"
  VERBATIM)
