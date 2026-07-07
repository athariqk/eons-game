set(KERNEL_TESTS
    include/ncore/kernel/memory.cpp include/ncore/kernel/collection.cpp
    include/ncore/kernel/reference.cpp)

set(EVENTS_BENCHMARKS "include/ncore/modules/events/event_bus_benchmark.cpp")

add_executable(ncore_test main.cpp ${KERNEL_TESTS} ${EVENTS_BENCHMARKS})

target_link_libraries(ncore_test PRIVATE Catch2::Catch2 ncore)

target_include_directories(
  ncore_test PRIVATE ${PROJECT_SOURCE_DIR}/ncore/include
                     ${PROJECT_SOURCE_DIR}/ncore/src)

list(APPEND CMAKE_MODULE_PATH
     "${PROJECT_SOURCE_DIR}/ncore/external/catch2/extras")

include(CTest)
include(Catch)
catch_discover_tests(ncore_test)
