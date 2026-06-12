add_executable(eons
    "main.cpp"
    "pch.h"
    "src/microcosmos/Evolution.h" "src/microcosmos/Evolution.cpp"
    "src/microcosmos/Genes.h"
    "src/microcosmos/MicrocosmWorld.h" "src/microcosmos/MicrocosmWorld.cpp"
    "src/microcosmos/components/FoodComponent.h"
    "src/microcosmos/components/OrganismAIComponent.h" "src/microcosmos/components/OrganismAIComponent.cpp"
    "src/microcosmos/components/OrganismComponent.h" "src/microcosmos/components/OrganismComponent.cpp"
    "src/microcosmos/components/SpeciesComponent.h" "src/microcosmos/components/SpeciesComponent.cpp"
    "src/microcosmos/systems/FoodSystem.h" "src/microcosmos/systems/FoodSystem.cpp"
    "src/microcosmos/systems/OrganismAISystem.h" "src/microcosmos/systems/OrganismAISystem.cpp"
    "src/microcosmos/systems/OrganismSystem.h" "src/microcosmos/systems/OrganismSystem.cpp"
    "src/microcosmos/systems/SpeciesSystem.h" "src/microcosmos/systems/SpeciesSystem.cpp"
    "src/experimental/NeuralNetwork/neural_network.h" "src/experimental/NeuralNetwork/neural_network.cpp"
)

target_include_directories(eons PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_link_libraries(eons PRIVATE "ncore")

target_precompile_headers(eons PRIVATE "pch.h")

set_target_properties(eons PROPERTIES
    DEBUG_POSTFIX "_d"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:eons>"
)

target_compile_definitions(eons PRIVATE
    "$<$<CONFIG:Debug>:DEBUG>"
    "$<$<CONFIG:Release>:RELEASE>"
    "$<$<CONFIG:Dist>:DIST>"
)

add_custom_command(
    TARGET eons POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${CMAKE_SOURCE_DIR}"
        "-DTARGET_DIR=$<TARGET_FILE_DIR:eons>"
        -P "${CMAKE_SOURCE_DIR}/eons/cmake/copy_files.cmake"
    COMMENT "Copying assets and config files"
    VERBATIM
)
