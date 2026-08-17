add_executable(EonsPrototype)

target_sources(
  EonsPrototype
  PRIVATE "main.cpp"
          "src/alloc_overrides.cpp"
          "src/microcosmos/Evolution.cpp"
          "src/microcosmos/MicrocosmModule.cpp"
          "src/microcosmos/SpeciesRegistry.cpp"
          "src/microcosmos/components/OrganismAIComponent.cpp"
          "src/microcosmos/components/OrganismComponent.cpp"
          "src/microcosmos/components/SpeciesComponent.cpp"
          "src/microcosmos/systems/FoodSystem.cpp"
          "src/microcosmos/systems/InitSystem.cpp"
          "src/microcosmos/systems/OrganismAISystem.cpp"
          "src/microcosmos/systems/OrganismSystem.cpp"
          "src/microcosmos/systems/SpeciesGuiSystem.cpp"
          "src/microcosmos/systems/SpeciesSystem.cpp"
          "src/experimental/NeuralNetwork/neural_network.cpp")

target_include_directories(EonsPrototype
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(EonsPrototype PRIVATE ncore ncore_editor)

target_precompile_headers(EonsPrototype PRIVATE "pch.h")

set_target_properties(
  EonsPrototype
  PROPERTIES
    DEBUG_POSTFIX "_d"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:EonsPrototype>"
    VS_DEBUGGER_ENVIRONMENT
    "ASAN_OPTIONS=fast_unwind_on_malloc=0:malloc_context_size=20:halt_on_error=0;ASAN_SYMBOLIZER_PATH=C:/tools/LLVM/bin/llvm-symbolizer.exe"
)

# ASAN runtime DLL must sit next to the exe (Debug + clang-cl only).
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT
                                             STREQUAL "MSVC")
  get_filename_component(_llvm_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
  set(_asan_runtime
      "${_llvm_bin_dir}/../lib/clang/${CMAKE_CXX_COMPILER_VERSION_MAJOR}/lib/windows/clang_rt.asan_dynamic-x86_64.dll"
  )
  if(NOT EXISTS "${_asan_runtime}")
    file(
      GLOB
      _asan_rt_candidates
      "${_llvm_bin_dir}/../lib/clang/*/lib/windows/clang_rt.asan_dynamic-x86_64.dll"
    )
    list(LENGTH _asan_rt_candidates _asan_rt_count)
    if(_asan_rt_count GREATER 0)
      list(GET _asan_rt_candidates 0 _asan_runtime)
    endif()
  endif()
  if(EXISTS "${_asan_runtime}")
    add_custom_command(
      TARGET EonsPrototype
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_asan_runtime}"
              "$<TARGET_FILE_DIR:EonsPrototype>"
      COMMENT "Copying ASAN runtime DLL")
  else()
    message(
      WARNING
        "ASAN runtime DLL not found at ${_asan_runtime} — Debug builds will fail to start"
    )
  endif()
endif()

target_compile_definitions(EonsPrototype PRIVATE "$<$<CONFIG:Debug>:DEBUG>"
                                                 "$<$<CONFIG:Dist>:DIST>")

add_custom_command(
  TARGET EonsPrototype
  POST_BUILD
  COMMAND
    "${CMAKE_COMMAND}" -E copy_directory_if_different
    "${CMAKE_SOURCE_DIR}/eons-game/assets"
    "$<TARGET_FILE_DIR:EonsPrototype>/assets"
  COMMAND
    "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_SOURCE_DIR}/eons-game/eons.ini"
    "$<TARGET_FILE_DIR:EonsPrototype>/eons.ini"
  COMMENT "Copying assets and config files"
  VERBATIM)
