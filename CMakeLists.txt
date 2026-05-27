cmake_minimum_required(VERSION 3.16)
project(eons LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define build configurations
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "Configs" FORCE)

# Output directories
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})

add_subdirectory(vendors)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/vendors/imgui)

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
set_property(TARGET eons PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIR}")

# Add Debug Configuration
set_target_properties(eons PROPERTIES DEBUG_POSTFIX "_d")
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

# Add Release Configuration
add_compile_definitions($<$<CONFIG:Release>:RELEASE>)
add_compile_options($<$<CONFIG:Release>:-O3>)

# Add Distribution Configuration
add_compile_definitions($<$<CONFIG:Dist>:DIST>)
add_compile_options($<$<CONFIG:Dist>:-O3>)

target_include_directories(eons PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/Engine
)

file(GLOB ASSETS "assets/*")
file(COPY ${ASSETS} DESTINATION "${OUTPUT_DIR}/assets/")
