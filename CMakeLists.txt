cmake_minimum_required(VERSION 3.27)

project(eons-project LANGUAGES C CXX)

enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJECT_PATH_MAX 200)

option(SEA_TRACK_ASAN "Compile SeaEngine project with Address Sanitizer" OFF)

set(CMAKE_CONFIGURATION_TYPES
    "Debug;Release;Dist"
    CACHE STRING "Configs" FORCE)

foreach(LANG C CXX EXE_LINKER SHARED_LINKER STATIC_LINKER MODULE_LINKER)
  set(CMAKE_${LANG}_FLAGS_DIST
      "${CMAKE_${LANG}_FLAGS_RELEASE}"
      CACHE STRING "${LANG} flags for Dist" FORCE)
endforeach()

# Compiler cache
find_program(SCCACHE sccache)
if(SCCACHE)
  set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE}")
  message(STATUS "sccache found — compiler caching enabled")
endif()

# Find LLVM installation for ASAN runtime libraries.
if(SEA_TRACK_ASAN)
  find_package(LLVM REQUIRED CONFIG)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/W4 /wd4146 /wd4245 /wd5105 /Zc:preprocessor)
  string(APPEND CMAKE_CXX_FLAGS " /std:c++20")
  if(SEA_TRACK_ASAN)
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob0 /Od")
  else()
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob0 /Od /RTC1")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-Wno-unknown-argument -Wno-c++98-compat
                      -Wno-c++98-compat-pedantic)
  string(APPEND CMAKE_CXX_FLAGS " /std:c++20")
  set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob0 /Od")
endif()

if(SEA_TRACK_ASAN)
  # this will cause mimalloc bypass, which routes to ASAN-tracked malloc
  # instead.
  add_compile_definitions(NC_ASAN=1)

  # The /fsanitize=address flag lives in target-level compile options (not in
  # the global *_FLAGS_DEBUG), so CMake try_compile checks — e.g. abseil's
  # _MSVC_LANG probe, which builds in the Debug config — do not inherit it. They
  # would otherwise hit "invalid argument '-MDd' not allowed with
  # '-fsanitize=address'" and poison the whole configure.
  add_compile_options(/fsanitize=address)

  # clang-cl rejects the debug CRT (/MDd) with /fsanitize=address, so the Debug
  # config uses the release dynamic CRT (/MD) for all targets. CMake injects the
  # runtime flag per-target from CMAKE_MSVC_RUNTIME_LIBRARY, so no /MDd may
  # appear anywhere in the Debug compile flags (Release/Dist are unchanged).
  set(CMAKE_C_FLAGS_DEBUG "/Zi /Ob0 /Od")
  set(CMAKE_MSVC_RUNTIME_LIBRARY
      "MultiThreadedDLL"
      CACHE STRING "MSVC runtime library (Debug uses release CRT for ASAN)"
            FORCE)

  # AddressSanitizer runtime wiring.
  #
  # clang-cl: CMake invokes lld-link directly, which has no /fsanitize flag, so
  # replicate exactly what the clang driver adds (runtime import lib, SEH
  # interceptor, and the whole-archive thunk lib that redirects CRT malloc/free
  # to the sanitizer). ASAN also requires non-incremental linking. MSVC cl.exe:
  # link.exe supports /fsanitize=address natively.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
     AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(_asan_rt_dir "${LLVM_INSTALL_PREFIX}/lib/windows")
    if(EXISTS "${_asan_rt_dir}/clang_rt.asan_dynamic-x86_64.lib")
      add_link_options(
        /libpath:${_asan_rt_dir}
        clang_rt.asan_dynamic-x86_64.lib
        -include:__asan_seh_interceptor
        -wholearchive:${_asan_rt_dir}/clang_rt.asan_dynamic_runtime_thunk-x86_64.lib
      )
      string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " /INCREMENTAL:NO")
      string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG " /INCREMENTAL:NO")
    else()
      message(
        WARNING
          "ASAN runtime libs not found at ${_asan_rt_dir} — Debug links will fail"
      )
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " /fsanitize=address")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG " /fsanitize=address")
  endif()
endif()

if(MINGW)
  message(STATUS "MinGW detected — static linking libgcc/libstdc++")
  add_link_options(-static-libgcc -static-libstdc++)
endif()

set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")

foreach(CONFIG Debug Release Dist)
  string(TOUPPER ${CONFIG} CONFIG_UPPER)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_DIR}/${CONFIG}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_DIR}/${CONFIG}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER}
      "${CMAKE_BINARY_DIR}/lib/${CONFIG}")
endforeach()

include(${CMAKE_SOURCE_DIR}/cmake/ReflectionGen.cmake)

add_subdirectory(tools/reflection_gen) # ncore depends on this
add_subdirectory(ncore)
add_subdirectory(tools/editor)
add_subdirectory(tools/resource_compiler)
add_subdirectory(eons-game)
add_subdirectory(seaengine)

# --- Offline asset pipeline --------------------------------------------------
# Stages raw sources into the runtime output dir (skipping every file the
# pipeline compiles, so raw originals never ship) and compiles them to .bin
# there (see cmake/CompileAssets.cmake). This is the only step that writes
# assets/; runs every build, staleness checks keep no-op runs to a stat sweep.
set(EONS_ASSET_SRC_DIRS
    "${CMAKE_SOURCE_DIR}/ncore/assets"
    "${CMAKE_SOURCE_DIR}/eons-game/assets"
    "${CMAKE_SOURCE_DIR}/seaengine/assets")
add_custom_target(
  asset_pipeline
  ALL
  COMMAND
    ${CMAKE_COMMAND}
    -DASSET_COMPILER=$<TARGET_FILE:resource_compiler>
    -DBIN_DIR=$<TARGET_FILE_DIR:EonsPrototype>
    "-DSRC_DIRS=${EONS_ASSET_SRC_DIRS}"
    -P ${CMAKE_SOURCE_DIR}/cmake/CompileAssets.cmake
  COMMENT "Compiling assets to .bin"
  VERBATIM)
add_dependencies(asset_pipeline resource_compiler)
add_dependencies(EonsPrototype asset_pipeline)
add_dependencies(SeaEngine asset_pipeline)

# --- Install rules ---
install(
  TARGETS ncore
  RUNTIME DESTINATION .
  ARCHIVE DESTINATION .
  LIBRARY DESTINATION .)
install(
  FILES $<TARGET_PDB_FILE:ncore>
  DESTINATION .
  OPTIONAL)

install(TARGETS EonsPrototype RUNTIME DESTINATION .)
install(
  FILES $<TARGET_PDB_FILE:EonsPrototype>
  DESTINATION .
  OPTIONAL)

install(
  TARGETS ncore_editor
  RUNTIME DESTINATION .
  ARCHIVE DESTINATION .
  LIBRARY DESTINATION .)
install(
  FILES $<TARGET_PDB_FILE:ncore_editor>
  DESTINATION .
  OPTIONAL)

install(FILES eons-game/eons.ini DESTINATION .)
install(DIRECTORY eons-game/assets/ DESTINATION assets)
