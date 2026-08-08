# NCORE engine library TODO: separate into dedicated project in the future

# option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

add_library(ncore SHARED)

set(NCORE_VERSION_STABLE 0)
set(NCORE_VERSION_MAJOR 0)
set(NCORE_VERSION_MINOR 1)
set(NCORE_VERSION_PATCH 0)

set(NCORE_VERSION_STR
    ${NCORE_VERSION_STABLE}.${NCORE_VERSION_MAJOR}.${NCORE_VERSION_MINOR}.${NCORE_VERSION_PATCH}
)
set(NCORE_VERSION
    ${NCORE_VERSION_STABLE}.${NCORE_VERSION_MAJOR}${NCORE_VERSION_MINOR}${NCORE_VERSION_PATCH}
)

option(ENABLE_PCH "Enable Precompiled Headers performance optimization" ON)

if(ENABLE_PCH)
  target_precompile_headers(ncore PRIVATE "pch.h")
endif()

if(MSVC)
  target_sources(ncore PRIVATE "${CMAKE_SOURCE_DIR}/ncore/ncore.natvis")
endif()

# todo: add this back later set_property(TARGET ncore PROPERTY UNITY_BUILD ON)
# set_property(TARGET ncore PROPERTY UNITY_BUILD_MODE BATCH)

target_compile_definitions(ncore PRIVATE "_CRT_SECURE_NO_WARNINGS")

set_target_properties(
  ncore PROPERTIES DEBUG_POSTFIX "_d" VS_DEBUGGER_WORKING_DIRECTORY
                                      "$<TARGET_FILE_DIR:ncore>")

# build config macros
target_compile_definitions(
  ncore PRIVATE $<$<CONFIG:Debug>:NC_DEBUG=1> $<$<CONFIG:Release>:NC_RELEASE=1>
                $<$<CONFIG:Dist>:NC_DIST=1>)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(ncore PRIVATE /GR- /w14701 /w14703)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(
    ncore
    PRIVATE -Wall
            -Wextra
            -Werror
            -Wconversion
            -Wdouble-promotion
            -Wuninitialized
            -Wno-unused-parameter
            -Wno-unused-function
            -Wno-sign-conversion
            -Wno-padded
            -Wno-global-constructors
            -Wno-unsafe-buffer-usage
            -Wno-switch-default
            -Wno-invalid-offsetof
            -Wno-switch-enum
            -fsanitize=undefined
            -fsanitize-trap)
endif()

target_include_directories(
  ncore SYSTEM
  PRIVATE "${CMAKE_SOURCE_DIR}/ncore/external/imgui"
          "${CMAKE_SOURCE_DIR}/ncore/external/box2d/include"
          "${CMAKE_SOURCE_DIR}/ncore/external/sdl/include"
          "${CMAKE_SOURCE_DIR}/ncore/external/inifile-cpp"
          "${CMAKE_SOURCE_DIR}/ncore/external/flecs/include")

add_subdirectory(external)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tooling)
add_subdirectory(tests EXCLUDE_FROM_ALL)

# Copy runtime DLLs alongside ncore target in the build tree
if(NOT NCORE_STATIC_DEPS)
  add_custom_command(
    TARGET ncore
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3-shared>" "$<TARGET_FILE_DIR:ncore>"
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:Diligent-GraphicsEngineVk-shared>"
      "$<TARGET_FILE_DIR:ncore>"
    COMMENT "Copying runtime DLLs (SDL3, Diligent)")
  if(SLANG_RUNTIME)
    add_custom_command(
      TARGET ncore
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SLANG_RUNTIME}"
              "$<TARGET_FILE_DIR:ncore>")
  endif()

  add_custom_command(
    TARGET ncore
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:ncore>/assets"
    COMMENT "Copying engine assets to output directory")
endif()

# DiligentCore dlls copy_required_dlls(ncore)

# DOCUMENTATION ## bits adapted from
# https://devblogs.microsoft.com/cppblog/clear-functional-c-documentation-with-sphinx-breathe-doxygen-cmake/

# doxygen
find_package(Doxygen REQUIRED)

if(DOXYGEN_FOUND)
  set(DOXYGEN_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen)
  set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen)

  file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
                 ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(
    Doxygen
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen")
endif()

# sphinx
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(Sphinx REQUIRED)

if(SPHINX_FOUND)
  add_custom_target(
    Sphinx ALL
    COMMAND
      ${SPHINX_EXECUTABLE} -b html
      -Dbreathe_projects.NCORE=${DOXYGEN_OUTPUT_DIR}/xml
      ${CMAKE_CURRENT_SOURCE_DIR}/docs/sphinx
      ${CMAKE_CURRENT_BINARY_DIR}/docs/sphinx
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating documentation with Sphinx")
endif()
