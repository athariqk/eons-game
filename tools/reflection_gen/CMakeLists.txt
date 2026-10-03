project(reflection_gen LANGUAGES CXX)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_executable(reflection_gen
    main.cpp
    ast_visitor.h
    ast_visitor.cpp
    code_generator.h
    code_generator.cpp
    llvm_instantiations.cpp)

target_compile_definitions(reflection_gen PRIVATE
    _CRT_SECURE_NO_WARNINGS LLVM_BUILD_STATIC)

target_compile_options(reflection_gen PRIVATE /O2)

set_property(TARGET reflection_gen PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

set_target_properties(reflection_gen
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

llvm_map_components_to_libnames(LLVM_LIBS
    support core irreader bitreader
    option asmparser)

target_link_libraries(reflection_gen PRIVATE
    clangAST
    clangASTMatchers
    clangBasic
    clangFrontend
    clangSerialization
    clangTooling
    clangToolingCore
    ${LLVM_LIBS})
