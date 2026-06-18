# ── Prebuilt vendor targets ──────────────────────────────────
# Activated by NC_USE_PREBUILT=ON.
# Points at the last build output in CMAKE_BINARY_DIR.

macro(_prebuilt_debug_libs)
    set(NC_BIN  "${CMAKE_BINARY_DIR}/bin/Debug")
    set(NC_LIB  "${CMAKE_BINARY_DIR}")
    find_file(NC_SDL3_DLL  SDL3.dll  PATHS "${NC_BIN}"  NO_DEFAULT_PATH)
    find_file(NC_SDL3_LIB  SDL3.lib  PATHS "${NC_BIN}"  NO_DEFAULT_PATH)

    # SDL3
    add_library(SDL3::SDL3 SHARED IMPORTED)
    set_target_properties(SDL3::SDL3 PROPERTIES
        IMPORTED_LOCATION    "${NC_SDL3_DLL}"
        IMPORTED_IMPLIB     "${NC_SDL3_LIB}"
    )
    add_library(SDL3::Headers INTERFACE IMPORTED)
    set_target_properties(SDL3::Headers PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}/sdl/include"
    )

    # Box2D
    find_file(NC_BOX2D_LIB  box2d  box2dd  PATHS "${NC_LIB}"  NO_DEFAULT_PATH)
    add_library(box2d::box2d STATIC IMPORTED)
    set_target_properties(box2d::box2d PROPERTIES
        IMPORTED_LOCATION    "${NC_BOX2D_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}/box2d/include"
    )

    # Flecs
    find_file(NC_FLECS_DLL  flecs.dll   PATHS "${NC_BIN}"  NO_DEFAULT_PATH)
    add_library(flecs::flecs SHARED IMPORTED)
    set_target_properties(flecs::flecs PROPERTIES
        IMPORTED_LOCATION    "${NC_FLECS_DLL}"
        IMPORTED_IMPLIB     "${NC_FLECS_DLL}.lib"
    )
    add_library(flecs_static STATIC IMPORTED)
    set_target_properties(flecs_static PROPERTIES
        IMPORTED_LOCATION    "${CMAKE_BINARY_DIR}/bin/Debug/flecs_static.lib"
    )
endmacro()

if(NOT NC_SDL3_DLL)
    _prebuilt_debug_libs()
endif()
