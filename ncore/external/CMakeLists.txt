# NCORE engine vendored libraries

# Disable all warnings for every buildsystem target at or below this directory.
# Called after all vendored subdirectories have been added so their targets
# exist. `-w`/`/w` is appended LAST to each target's options, overriding the
# lib's own /W2/-Wall/-Werror (which appear earlier in the command line).
function(nc_silence_targets dir)
  get_directory_property(_subs DIRECTORY ${dir} SUBDIRECTORIES)
  foreach(_s ${_subs})
    nc_silence_targets(${_s})
  endforeach()
  get_directory_property(_targets DIRECTORY ${dir} BUILDSYSTEM_TARGETS)
  foreach(_t ${_targets})
    get_target_property(_type ${_t} TYPE)
    if(_type STREQUAL "INTERFACE_LIBRARY" OR _type STREQUAL "UTILITY")
      continue()
    endif()
    get_target_property(_imported ${_t} IMPORTED)
    if(_imported)
      continue()
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
      target_compile_options(${_t} PRIVATE -w)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      target_compile_options(${_t} PRIVATE /w)
    endif()
  endforeach()
endfunction()

# mimalloc dep
add_subdirectory(mimalloc SYSTEM)
set(MI_SECURE ON CACHE BOOL "" FORCE)
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
set(MI_OVERRIDE OFF CACHE BOOL "" FORCE)

# ImGui
add_library(
  ImGui STATIC
  imgui/imgui.h
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui_internal.h
  imgui/imstb_rectpack.h
  imgui/imstb_textedit.h
  imgui/imstb_truetype.h
  imgui/misc/cpp/imgui_stdlib.cpp
  $<$<CONFIG:Debug>:imgui/imgui_demo.cpp>)
target_include_directories(ImGui SYSTEM PUBLIC imgui)
target_compile_features(ImGui PRIVATE cxx_std_17)
set_target_properties(ImGui PROPERTIES POSITION_INDEPENDENT_CODE ON)

# inifile-cpp
add_library(inifile-cpp INTERFACE inifile-cpp/inicpp.h)
target_include_directories(inifile-cpp SYSTEM INTERFACE inifile-cpp)

# dont build SDL tests and SDL as shared lib
set(SDL_TEST_LIBRARY
    OFF
    CACHE BOOL "" FORCE)
set(SDL_STATIC
    ON
    CACHE BOOL "" FORCE)
set(SDL_SHARED
    OFF
    CACHE BOOL "" FORCE)

# dont build flecs as shared lib
set(FLECS_SHARED
    OFF
    CACHE BOOL "" FORCE)

add_subdirectory(box2d SYSTEM)
add_subdirectory(sdl SYSTEM)
add_subdirectory(flecs SYSTEM)

add_subdirectory(diligent-core SYSTEM)

# minimal DiligentCore build
set(DILIGENT_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(DILIGENT_NO_FORMAT_VALIDATION
    ON
    CACHE BOOL "" FORCE)
set(DILIGENT_NO_METAL
    ON
    CACHE BOOL "" FORCE)
set(DILIGENT_NO_WEBGPU
    ON
    CACHE BOOL "" FORCE)
set(DILIGENT_NO_DIRECT3D11
    ON
    CACHE BOOL "" FORCE)
set(DILIGENT_NO_DIRECT3D12
    ON
    CACHE BOOL "" FORCE)
set(DILIGENT_NO_OPENGL
    ON
    CACHE BOOL "" FORCE)

target_link_libraries(
  ncore
  PRIVATE mimalloc-static
		  ImGui
          box2d::box2d
          SDL3::SDL3-static
          inifile-cpp
          flecs::flecs_static
		  Diligent-Common
          Diligent-GraphicsEngineVk-static
          Diligent-GraphicsEngineVkInterface
          Diligent-GraphicsTools
		  Diligent-GraphicsAccessories)

# tests
add_subdirectory(catch2 SYSTEM EXCLUDE_FROM_ALL)

# Silence warnings from all vendored targets defined above. Must run after
# every add_subdirectory so all targets exist.
nc_silence_targets(${CMAKE_CURRENT_SOURCE_DIR})
