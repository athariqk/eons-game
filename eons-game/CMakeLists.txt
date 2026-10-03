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
if(SEA_TRACK_ASAN AND CMAKE_CXX_COMPILER_ID MATCHES "Clang"
   AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
  set(_asan_runtime
      "${LLVM_INSTALL_PREFIX}/lib/windows/clang_rt.asan_dynamic-x86_64.dll")
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

# Match ncore/editor: RTTI ADD_PROPERTY uses offsetof on types that may not be
# standard-layout (e.g. std::string members) — conditionally-supported, not a bug.
target_compile_options(
  EonsPrototype
  PRIVATE $<$<CXX_COMPILER_ID:Clang>:
            -Wno-invalid-offsetof
            -Wno-unsafe-buffer-usage
            -Wno-lifetime-safety-intra-tu-suggestions
            -Wno-lifetime-safety-invalidation>)

# Reflection code generation — registers game component types (TestSpin,
# microcosmos) so EcsWorld::add_component/find<T>() can resolve them.
file(GLOB_RECURSE EONS_GAME_HEADERS
     LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

nc_add_reflection_gen(EonsPrototype HEADERS ${EONS_GAME_HEADERS}
                      FLAGS_FROM "eons-game/")

# .gen.h files re-include their source header by relative path; headers at the
# game root (test_spin.h, pch.h) resolve as bare <test_spin.h>.
target_include_directories(EonsPrototype PRIVATE
                           "${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_command(
  TARGET EonsPrototype
  POST_BUILD
  COMMAND
    "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_SOURCE_DIR}/eons-game/eons.ini"
    "$<TARGET_FILE_DIR:EonsPrototype>/eons.ini"
  COMMENT "Copying config files"
  VERBATIM)
