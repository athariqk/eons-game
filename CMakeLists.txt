cmake_minimum_required(VERSION 3.27)

project(eons-project LANGUAGES C CXX)

enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/W4 /wd4146 /wd4245 /wd5105 /Zc:preprocessor)
  set(CMAKE_CXX_FLAGS_DEBUG "/MDd /ZI /Ob0 /Od /RTC1")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-Wno-unknown-argument -Wno-c++98-compat
                      -Wno-c++98-compat-pedantic)
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

add_subdirectory(ncore)
add_subdirectory(eons-game)
