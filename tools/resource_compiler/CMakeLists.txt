add_executable(resource_compiler)

target_sources(resource_compiler PRIVATE "main.cpp")
target_link_libraries(resource_compiler PUBLIC ncore)
