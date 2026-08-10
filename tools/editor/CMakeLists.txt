add_library(ncore_editor SHARED)

target_sources(ncore_editor PRIVATE
    ncore_editor.h
    ncore_editor.cpp
    imgui_style.h
)

target_link_libraries(ncore_editor PUBLIC ncore)
target_link_libraries(ncore_editor PRIVATE ImGui freetype)

target_include_directories(ncore_editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

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
