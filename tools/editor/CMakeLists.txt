add_library(ncore_editor SHARED)

target_sources(ncore_editor PRIVATE
    include/editor/ncore_editor.h
    include/editor/imgui_style.h
    src/editor_state.h
    src/ncore_editor.cpp
)

set(IMGUIZMO_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)

add_subdirectory(external/imguizmo)

target_link_libraries(ncore_editor PUBLIC ncore)
target_link_libraries(imguizmo PUBLIC ImGui)
target_link_libraries(ncore_editor PRIVATE ImGui imguizmo freetype)

target_include_directories(ncore_editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(ncore_editor PROPERTIES
    DEBUG_POSTFIX "_d"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:ncore_editor>"
)

target_compile_definitions(ncore_editor PRIVATE
    $<$<CONFIG:Debug>:NC_DEBUG=1>
    $<$<CONFIG:Release>:NC_RELEASE=1>
    $<$<CONFIG:Dist>:NC_DIST=1>
)

add_custom_command(TARGET ncore_editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:ncore_editor>" "$<TARGET_FILE_DIR:eons-game>"
)
