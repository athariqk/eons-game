add_executable(SeaEngine)

target_sources(SeaEngine PRIVATE main.cpp src/world_scene.h src/world_scene.cpp)

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
