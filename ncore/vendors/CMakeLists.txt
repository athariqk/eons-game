# NCORE engine vendored libraries

option(NC_USE_PREBUILT "Skip rebuilding heavy dependencies (SDL/Box2D/Flecs)" OFF)
set(NC_PREBUILT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/${CMAKE_BUILD_TYPE}")

# define IMPORTED targets from prebuilt binaries
macro(_add_prebuilt_sdl)
    if(NOT EXISTS "${NC_PREBUILT_DIR}/SDL3.dll")
        message(FATAL_ERROR "Prebuilt SDL3 not found at ${NC_PREBUILT_DIR}/SDL3.dll")
    endif()
    add_library(SDL3::SDL3 SHARED IMPORTED)
    set_target_properties(SDL3::SDL3 PROPERTIES
        IMPORTED_LOCATION          "${NC_PREBUILT_DIR}/SDL3.dll"
        IMPORTED_IMPLIB           "${NC_PREBUILT_DIR}/SDL3.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/sdl/include"
    )
    add_library(SDL3::Headers INTERFACE IMPORTED)
    set_target_properties(SDL3::Headers PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/sdl/include"
    )
endmacro()

macro(_add_prebuilt_box2d)
    if(NOT EXISTS "${NC_PREBUILT_DIR}/box2d${CMAKE_DEBUG_POSTFIX}.lib")
        message(FATAL_ERROR "Prebuilt Box2D not found at ${NC_PREBUILT_DIR}/box2d.lib")
    endif()
    add_library(box2d::box2d STATIC IMPORTED)
    set_target_properties(box2d::box2d PROPERTIES
        IMPORTED_LOCATION          "${NC_PREBUILT_DIR}/box2d${CMAKE_DEBUG_POSTFIX}.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/box2d/include"
    )
endmacro()

macro(_add_prebuilt_flecs)
    if(NOT EXISTS "${NC_PREBUILT_DIR}/flecs.dll")
        message(FATAL_ERROR "Prebuilt Flecs not found at ${NC_PREBUILT_DIR}/flecs.dll")
    endif()
    add_library(flecs::flecs SHARED IMPORTED)
    set_target_properties(flecs::flecs PROPERTIES
        IMPORTED_LOCATION          "${NC_PREBUILT_DIR}/flecs.dll"
        IMPORTED_IMPLIB           "${NC_PREBUILT_DIR}/flecs.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/flecs/include"
    )
endmacro()

# ImGui
add_library(ImGui STATIC
    imgui/imgui.h imgui/imgui.cpp
    imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp
    imgui/imgui_internal.h
    imgui/imstb_rectpack.h imgui/imstb_textedit.h imgui/imstb_truetype.h
    imgui/misc/cpp/imgui_stdlib.cpp
    $<$<CONFIG:Debug>:imgui/imgui_demo.cpp>
)
target_include_directories(ImGui PUBLIC imgui)
target_compile_features(ImGui PRIVATE cxx_std_17)
set_target_properties(ImGui PROPERTIES POSITION_INDEPENDENT_CODE ON)

# inifile-cpp
add_library(inifile-cpp INTERFACE inifile-cpp/inicpp.h)
target_include_directories(inifile-cpp INTERFACE inifile-cpp)

if(NC_USE_PREBUILT)
    message(STATUS "Prebuilt vendor mode, using binaries from ${NC_PREBUILT_DIR}")
    _add_prebuilt_sdl()
    _add_prebuilt_box2d()
    _add_prebuilt_flecs()
else()
    add_subdirectory(box2d)
    add_subdirectory(sdl)
    add_subdirectory(flecs)
endif()

target_link_libraries(ncore PRIVATE
    ImGui box2d::box2d SDL3::SDL3 SDL3::Headers inifile-cpp flecs::flecs
)
