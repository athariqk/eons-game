cmake_minimum_required(VERSION 3.16)
project(eons LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define build configurations
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "Configs" FORCE)
set(CMAKE_C_FLAGS_DIST "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "C flags for Dist builds" FORCE)
set(CMAKE_CXX_FLAGS_DIST "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "C++ flags for Dist builds" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DIST "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE STRING "Executable linker flags for Dist builds" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_DIST "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "Shared linker flags for Dist builds" FORCE)
set(CMAKE_STATIC_LINKER_FLAGS_DIST "${CMAKE_STATIC_LINKER_FLAGS_RELEASE}" CACHE STRING "Static linker flags for Dist builds" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS_DIST "${CMAKE_MODULE_LINKER_FLAGS_RELEASE}" CACHE STRING "Module linker flags for Dist builds" FORCE)

if(MSVC)
    add_compile_options(/FS /wd5105)
endif()

# Output directories
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
foreach(CONFIG_TYPE Debug Release Dist)
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${OUTPUT_DIR}/${CONFIG_TYPE})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${OUTPUT_DIR}/${CONFIG_TYPE})
endforeach()

add_subdirectory(vendors)

# Source files
file(GLOB_RECURSE SRC_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.h
)

# Create executable
add_executable(eons ${SRC_FILES}
        src/Engine/Random.h)

# Define preprocessor macros
target_compile_definitions(eons PRIVATE _CRT_SECURE_NO_WARNINGS)

find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3-shared)

find_package(SDL3_image CONFIG REQUIRED)

# Link external libraries
target_link_libraries(eons PRIVATE
        ImGui
        box2d::box2d
        spdlog::spdlog
        SDL3::SDL3
        $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>
)

# Platform-specific settings
if(WIN32)
    set(CMAKE_SYSTEM_VERSION latest)
endif()

if(MINGW)
    message(STATUS "Detected MinGW, enabling static linking for libgcc and libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

# Build configurations
set_property(TARGET eons PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:eons>")

# Add Debug Configuration
set_target_properties(eons PROPERTIES DEBUG_POSTFIX "_d")
target_compile_definitions(eons PRIVATE $<$<CONFIG:Debug>:DEBUG>)

# Add Release Configuration
target_compile_definitions(eons PRIVATE $<$<CONFIG:Release>:RELEASE>)
target_compile_options(eons PRIVATE
        $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2>
        $<$<AND:$<CONFIG:Release>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-O3>
)

# Add Distribution Configuration
target_compile_definitions(eons PRIVATE $<$<CONFIG:Dist>:DIST>)
target_compile_options(eons PRIVATE
        $<$<AND:$<CONFIG:Dist>,$<CXX_COMPILER_ID:MSVC>>:/O2>
        $<$<AND:$<CONFIG:Dist>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-O3>
)

target_include_directories(eons PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/Engine
        ${CMAKE_SOURCE_DIR}/vendors/imgui
)

add_custom_command(TARGET eons POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:eons>/assets
)
