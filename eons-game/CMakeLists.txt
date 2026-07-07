add_executable(eons-game)

target_sources(
  eons-game
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

target_include_directories(eons-game PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(eons-game PRIVATE ncore)

target_precompile_headers(eons-game PRIVATE "pch.h")

set_target_properties(
  eons-game PROPERTIES DEBUG_POSTFIX "_d" VS_DEBUGGER_WORKING_DIRECTORY
                                          "$<TARGET_FILE_DIR:eons-game>")

target_compile_definitions(
  eons-game PRIVATE "$<$<CONFIG:Debug>:DEBUG>" "$<$<CONFIG:Release>:RELEASE>"
                    "$<$<CONFIG:Dist>:DIST>")

add_custom_command(
  TARGET eons-game
  POST_BUILD
  COMMAND
    "${CMAKE_COMMAND}" -E copy_directory_if_different
    "${CMAKE_SOURCE_DIR}/eons-game/assets" "$<TARGET_FILE_DIR:eons-game>/assets"
  COMMAND
    "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_SOURCE_DIR}/eons-game/eons.ini"
    "$<TARGET_FILE_DIR:eons-game>/eons.ini"
  COMMENT "Copying assets and config files"
  VERBATIM)
