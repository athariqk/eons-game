# NCORE engine library
# TODO: separate into dedicated project in the future

add_library(ncore STATIC "pch.h")

set(NCORE_VERSION_STABLE 0)
set(NCORE_VERSION_MAJOR 0)
set(NCORE_VERSION_MINOR 1)
set(NCORE_VERSION_PATCH 0)

set(NCORE_VERSION_STR ${NCORE_VERSION_STABLE}.${NCORE_VERSION_MAJOR}.${NCORE_VERSION_MINOR}.${NCORE_VERSION_PATCH})
set(NCORE_VERSION ${NCORE_VERSION_STABLE}.${NCORE_VERSION_MAJOR}${NCORE_VERSION_MINOR}${NCORE_VERSION_PATCH})

option(ENABLE_PCH "Enable Precompiled Headers performance optimization" ON)

if(ENABLE_PCH)
	target_precompile_headers(ncore PRIVATE "pch.h")
endif()

# todo: add this back later
#set_property(TARGET ncore PROPERTY UNITY_BUILD ON)
#set_property(TARGET ncore PROPERTY UNITY_BUILD_MODE BATCH)

target_compile_definitions(ncore PRIVATE "_CRT_SECURE_NO_WARNINGS")

set_target_properties(ncore PROPERTIES
	DEBUG_POSTFIX "_d"
	VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:ncore>"
)

# build config macros
target_compile_definitions(ncore PRIVATE
    $<$<CONFIG:Debug>:NC_DEBUG=1>
    $<$<CONFIG:Release>:NC_RELEASE=1>
    $<$<CONFIG:Dist>:NC_DIST=1>
)

target_compile_options(ncore PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/GR->")

target_include_directories(ncore PRIVATE
    "${CMAKE_SOURCE_DIR}/ncore/vendors/imgui"
    "${CMAKE_SOURCE_DIR}/ncore/vendors/box2d/include"
    "${CMAKE_SOURCE_DIR}/ncore/vendors/sdl/include"
    "${CMAKE_SOURCE_DIR}/ncore/vendors/inifile-cpp"
    "${CMAKE_SOURCE_DIR}/ncore/vendors/flecs/include"
)

add_subdirectory(vendors)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tests)


## DOCUMENTATION ##
# bits adapted from https://devblogs.microsoft.com/cppblog/clear-functional-c-documentation-with-sphinx-breathe-doxygen-cmake/

# doxygen
find_package(Doxygen REQUIRED)

if(DOXYGEN_FOUND)
	set(DOXYGEN_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen)
	set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen)

	file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(Doxygen ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	                   COMMENT "Generating API documentation with Doxygen")
endif()

# sphinx
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(Sphinx REQUIRED)

if(SPHINX_FOUND)
	add_custom_target(Sphinx ALL
	                  COMMAND
	                  ${SPHINX_EXECUTABLE} -b html
					  -Dbreathe_projects.NCORE=${DOXYGEN_OUTPUT_DIR}/xml
	                  ${CMAKE_CURRENT_SOURCE_DIR}/docs/sphinx ${CMAKE_CURRENT_BINARY_DIR}/docs/sphinx
	                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	                  COMMENT "Generating documentation with Sphinx")
endif()
