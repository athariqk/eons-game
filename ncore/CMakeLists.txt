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

if(WIN32)
  # dbghelp: PDB symbolization for the ASAN leak reporter
  target_link_libraries(ncore PRIVATE Dbghelp.lib)
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
  target_compile_options(ncore PRIVATE /GR- /w14701 /w14703 /utf-8)
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
			-Wno-nonportable-system-include-path
			-Wno-lifetime-safety-intra-tu-suggestions
			-Wno-lifetime-safety-invalidation
            # false positive: early error-returns next to an NRVO candidate
            # (llvm/llvm-project#158594)
            -Wno-nrvo
            -fsanitize=undefined
            -fsanitize-trap)
endif()

target_include_directories(
  ncore SYSTEM
  PRIVATE "${CMAKE_SOURCE_DIR}/ncore/external/imgui"
          "${CMAKE_SOURCE_DIR}/ncore/external/sdl/include"
          "${CMAKE_SOURCE_DIR}/ncore/external/stb"
          "${CMAKE_SOURCE_DIR}/ncore/external/inifile-cpp"
          "${CMAKE_SOURCE_DIR}/ncore/external/flecs/include"
          "${CMAKE_SOURCE_DIR}/ncore/external/diligent-core/ThirdParty/Vulkan-Headers/include")

add_subdirectory(external)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tests EXCLUDE_FROM_ALL)

# Reflection code generation
#
# Each header is processed individually via --single-header so that changing
# one header only re-parses that header through Clang, not all ~60 headers.

file(GLOB_RECURSE NCORE_PUBLIC_HEADERS
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/include/ncore/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

set(NCORE_GEN_DIR      "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(NCORE_REFLECTION_DB "${NCORE_GEN_DIR}/reflection_db")
set(NCORE_HEADERS_LIST  "${NCORE_GEN_DIR}/ncore_headers.txt")

# Header list for the filtered compile DB
file(GENERATE OUTPUT "${NCORE_HEADERS_LIST}"
     CONTENT "$<JOIN:${NCORE_PUBLIC_HEADERS},\n>")

# Filtered compilation database
add_custom_command(
    OUTPUT "${NCORE_REFLECTION_DB}/compile_commands.json"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NCORE_REFLECTION_DB}"
    COMMAND ${CMAKE_COMMAND}
            -DCOMPILE_COMMANDS=${CMAKE_BINARY_DIR}/compile_commands.json
            -DHEADERS_FILE=${NCORE_HEADERS_LIST}
            -DOUTPUT=${NCORE_REFLECTION_DB}/compile_commands.json
            -P ${CMAKE_SOURCE_DIR}/cmake/ReflectionCompileDb.cmake
    DEPENDS ${CMAKE_SOURCE_DIR}/cmake/ReflectionCompileDb.cmake
    COMMENT "Generating filtered compilation database for reflection_gen"
)

# Per-header generation
set(NCORE_GEN_HEADERS "")
foreach(header IN LISTS NCORE_PUBLIC_HEADERS)
    file(RELATIVE_PATH rel "${CMAKE_SOURCE_DIR}" "${header}")
    string(REGEX REPLACE "\\.h$" ".gen.h" gen_name "${rel}")
    string(REGEX REPLACE "[/\\\\]" "_" gen_name "${gen_name}")
    set(gen_header "${NCORE_GEN_DIR}/${gen_name}")

	add_custom_command(
	    OUTPUT "${gen_header}"
	    COMMAND reflection_gen
	            --single-header=${header}
	            --output=${gen_header}
	            -p=${NCORE_REFLECTION_DB}
	            --output-dir=${NCORE_GEN_DIR}
	    DEPENDS "${header}"
	            reflection_gen
	            "${NCORE_REFLECTION_DB}/compile_commands.json"
	    COMMENT "Generating reflection for ${rel}"
	    VERBATIM
	)
    list(APPEND NCORE_GEN_HEADERS "${gen_header}")
endforeach()

# Aggregate implementation unit (just #includes all .gen.h files)
add_custom_command(
    OUTPUT "${NCORE_GEN_DIR}/reflection_impl.cpp"
    COMMAND ${CMAKE_COMMAND}
            "-DOUTPUT_DIR=${NCORE_GEN_DIR}"
            "-DGEN_HEADERS=${NCORE_GEN_HEADERS}"
            -P ${CMAKE_SOURCE_DIR}/cmake/WriteReflectionImpl.cmake
    DEPENDS ${NCORE_GEN_HEADERS}
    COMMENT "Generating reflection implementation unit"
)

target_sources(ncore PRIVATE "${NCORE_GEN_DIR}/reflection_impl.cpp")
target_include_directories(ncore PRIVATE ${NCORE_GEN_DIR})

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
