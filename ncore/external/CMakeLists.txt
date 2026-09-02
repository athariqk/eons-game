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
set(MI_SECURE
    ON
    CACHE BOOL "" FORCE)
set(MI_BUILD_SHARED
    OFF
    CACHE BOOL "" FORCE)
set(MI_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(MI_BUILD_OBJECT
    OFF
    CACHE BOOL "" FORCE)
set(MI_OVERRIDE
    OFF
    CACHE BOOL "" FORCE)

# inifile-cpp
add_library(inifile-cpp INTERFACE inifile-cpp/inicpp.h)
target_include_directories(inifile-cpp SYSTEM INTERFACE inifile-cpp)

# SDL3
set(SDL_TEST_LIBRARY
    OFF
    CACHE BOOL "" FORCE)

# Flecs
set(FLECS_SHARED
    OFF
    CACHE BOOL "" FORCE)

option(
  NCORE_STATIC_DEPS
  "Use static linking for SDL, Diligent, and Slang (single-exe distribution)"
  OFF)

if(NCORE_STATIC_DEPS)
  # All static
  set(SDL_STATIC
      ON
      CACHE BOOL "" FORCE)
  set(SDL_SHARED
      OFF
      CACHE BOOL "" FORCE)
  set(DILIGENT_VK_SUFFIX "-static")
  set(SDL_TARGET SDL3::SDL3-static)
else()
  # All dynamic (default Debug/Release)
  set(SDL_STATIC
      OFF
      CACHE BOOL "" FORCE)
  set(SDL_SHARED
      ON
      CACHE BOOL "" FORCE)
  set(DILIGENT_VK_SUFFIX "-shared")
  set(SDL_TARGET SDL3::SDL3-shared)
endif()

add_subdirectory(sdl SYSTEM)
add_subdirectory(flecs SYSTEM)

# Minimal DiligentCore build
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
set(DILIGENT_NO_SUPER_RESOLUTION
    ON
    CACHE BOOL "" FORCE)
set(DILIGENT_NO_HLSL
    ON
    CACHE BOOL "" FORCE)

add_subdirectory(diligent-core SYSTEM)

# The ASAN Debug config uses the release CRT (/MD), which cannot link the
# CRT-debug-heap calls (_CrtSetDbgFlag, _malloc_dbg) that Diligent's
# BuildSettings enables via _DEBUG. Drop the _DEBUG/DEBUG defines from their
# interface — Diligent's own debug paths key off DILIGENT_DEBUG and
# DILIGENT_DEVELOPMENT (separate interface), so nothing is lost, and
# _ITERATOR_DEBUG_LEVEL stays consistent across all static libs.
if(TARGET Diligent-BuildSettings)
  set_property(
    TARGET Diligent-BuildSettings
    PROPERTY INTERFACE_COMPILE_DEFINITIONS
            "$<$<CONFIG:Release>:NDEBUG>")
endif()

# Slang
if(NCORE_STATIC_DEPS)
  # Static build from source via ExternalProject (Dist)
  include(ExternalProject)
  set(SLANG_STAGE_DIR "${CMAKE_BINARY_DIR}/_deps/slang-install")

  ExternalProject_Add(
    slang-project
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/slang
    CMAKE_ARGS -DSLANG_SLANG_LLVM_FLAVOR=DISABLE
               -DSLANG_ENABLE_DXIL=OFF
               -DSLANG_ENABLE_GFX=OFF
               -DSLANG_ENABLE_SLANGD=OFF
               -DSLANG_ENABLE_SLANGC=ON
               -DSLANG_ENABLE_SLANGRT=OFF
               -DSLANG_ENABLE_TESTS=OFF
               -DSLANG_ENABLE_EXAMPLES=OFF
               -DSLANG_ENABLE_REPLAYER=OFF
               -DSLANG_LIB_TYPE=STATIC
               -DCMAKE_INSTALL_PREFIX=${SLANG_STAGE_DIR}
               -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
               -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    INSTALL_DIR ${SLANG_STAGE_DIR}
    BUILD_BYPRODUCTS
      ${SLANG_STAGE_DIR}/lib/slang-compiler${CMAKE_STATIC_LIBRARY_SUFFIX})

  file(MAKE_DIRECTORY ${SLANG_STAGE_DIR}/include)
  file(MAKE_DIRECTORY ${SLANG_STAGE_DIR}/lib)

  add_library(slang::slang STATIC IMPORTED)
  add_dependencies(slang::slang slang-project)
  set_target_properties(
    slang::slang
    PROPERTIES
      IMPORTED_LOCATION
      ${SLANG_STAGE_DIR}/lib/slang-compiler${CMAKE_STATIC_LIBRARY_SUFFIX})
  target_include_directories(slang::slang INTERFACE ${SLANG_STAGE_DIR}/include)
else()
  # Dynamic linking from VulkanSDK (Debug / Release)
  find_path(
    SLANG_INCLUDE_DIR
    NAMES slang.h
    PATHS "$ENV{VULKAN_SDK}/Include/slang"
    NO_DEFAULT_PATH REQUIRED)
  find_library(
    SLANG_LIBRARY
    NAMES slang-compiler slang
    PATHS "$ENV{VULKAN_SDK}/Lib"
    NO_DEFAULT_PATH REQUIRED)
  find_file(
    SLANG_RUNTIME
    NAMES slang-compiler${CMAKE_SHARED_LIBRARY_SUFFIX}
          slang${CMAKE_SHARED_LIBRARY_SUFFIX}
    PATHS "$ENV{VULKAN_SDK}/Bin"
    NO_DEFAULT_PATH)
  set(SLANG_RUNTIME
      "${SLANG_RUNTIME}"
      CACHE FILEPATH "Slang runtime DLL path")

  add_library(slang::slang SHARED IMPORTED)
  set_target_properties(
    slang::slang PROPERTIES IMPORTED_LOCATION "${SLANG_RUNTIME}"
                            IMPORTED_IMPLIB "${SLANG_LIBRARY}")
  target_include_directories(slang::slang INTERFACE "${SLANG_INCLUDE_DIR}")

  install(FILES "${SLANG_RUNTIME}" DESTINATION .)
endif()

target_link_libraries(
  ncore
  PRIVATE mimalloc-static
          ${SDL_TARGET}
          inifile-cpp
          flecs::flecs_static
          Diligent-Common
          Diligent-GraphicsEngineVk${DILIGENT_VK_SUFFIX}
          Diligent-GraphicsEngineVkInterface
          Diligent-GraphicsTools
          Diligent-GraphicsAccessories
          slang::slang)

# Install runtime DLLs (dynamic-only)
if(NOT NCORE_STATIC_DEPS)
  install(FILES $<TARGET_FILE:${SDL_TARGET}> DESTINATION .)
  install(FILES $<TARGET_FILE:Diligent-GraphicsEngineVk-shared> DESTINATION .)
endif()

# tests
add_subdirectory(catch2 SYSTEM EXCLUDE_FROM_ALL)

# Silence warnings from all vendored targets defined above. Must run after every
# add_subdirectory so all targets exist.
nc_silence_targets(${CMAKE_CURRENT_SOURCE_DIR})
