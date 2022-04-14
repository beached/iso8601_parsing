cmake_policy(SET CMP0065 NEW)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_BUILD_TYPE EQUAL "Debug")
    add_compile_definitions(DEBUG=1)
endif ()

add_library(test_deps INTERFACE)
target_link_libraries(test_deps INTERFACE daw::daw-iso8601-parsing)
target_compile_features(test_deps INTERFACE cxx_std_17)

add_custom_target(${PROJECT_NAME}_full)

add_executable(iso8601_test iso8601_tests.cpp)
add_test(iso8601_test_tests iso8601_test)
target_link_libraries(iso8601_test PRIVATE test_deps)
add_dependencies(${PROJECT_NAME}_full iso8601_test)

add_executable(benchmarks benchmarks.cpp)
target_link_libraries(benchmarks PRIVATE test_deps)
add_dependencies(${PROJECT_NAME}_full benchmarks)

add_executable(small_test small_test.cpp)
add_test(small_test_test small_test)
target_link_libraries(small_test PRIVATE test_deps)
add_dependencies(${PROJECT_NAME}_full small_test)
