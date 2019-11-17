cmake_minimum_required(VERSION 3.12 )

project(iso8601_parsing)

#set( CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard whose features are requested.")

enable_testing()

include("${CMAKE_SOURCE_DIR}/dependent_projects/CMakeListsCompiler.txt")

include_directories("./include")
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

set(DAW_JSON_DONT_USE_EXCEPTIONS OFF CACHE STRING "Don't throw exceptions when json errors occur")
if (DAW_JSON_DONT_USE_EXCEPTIONS)
    if (DAW_JSON_DONT_USE_EXCEPTIONS=OFF)
        message("DAW_JSON_DONT_USE_EXCEPTIONS=OFF: Errors throw json_exception")
        add_compile_definitions(DAW_USE_JSON_EXCEPTIONS=true)
    else ()
        message("DAW_JSON_DONT_USE_EXCEPTIONS=ON: Errors abort program")
    endif ()
else ()
    message("DAW_JSON_DONT_USE_EXCEPTIONS=OFF: Errors throw json_exception")
    add_compile_definitions(DAW_USE_JSON_EXCEPTIONS=true)
endif ()

set(DAW_JSON_CHECK_DEBUG_ONLY OFF CACHE STRING "Only check for json errors in debug")
if (DAW_JSON_CHECK_DEBUG_ONLY)
    if (DAW_JSON_CHECK_DEBUG_ONLY=OFF)
        message("DAW_JSON_CHECK_DEBUG_ONLY=OFF: Always checking for valid json")
        add_compile_definitions(DAW_JSON_CHECK_ALWAYS=true)
    else ()
        message("DAW_JSON_CHECK_DEBUG_ONLY=ON: Only checking for valid json in debug")
    endif ()
else ()
    message("DAW_JSON_CHECK_DEBUG_ONLY=OFF: Always checking for valid json")
    add_compile_definitions(DAW_JSON_CHECK_ALWAYS=true)
endif ()

set(DEP_ROOT_DIR_BASE "${CMAKE_SOURCE_DIR}/dependent_projects/build")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(DEP_ROOT_DIR "${DEP_ROOT_DIR_BASE}/debug")
    set(GLEAN_CACHE "${CMAKE_SOURCE_DIR}/.glean/debug")
    set(GLEAN_CACHE2 "${CMAKE_SOURCE_DIR}/.glean/release")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(DEP_ROOT_DIR "${DEP_ROOT_DIR_BASE}/release")
    set(GLEAN_CACHE "${CMAKE_SOURCE_DIR}/.glean/release")
    set(GLEAN_CACHE2 "${CMAKE_SOURCE_DIR}/.glean/debug")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(DEP_ROOT_DIR "${DEP_ROOT_DIR_BASE}/release")
    set(GLEAN_CACHE "${CMAKE_SOURCE_DIR}/.glean/release")
    set(GLEAN_CACHE2 "${CMAKE_SOURCE_DIR}/.glean/debug")
endif ()

message("Building for: ${CMAKE_BUILD_TYPE}")
message("Checking for glean cache: '${GLEAN_CACHE}'")

option(GLEAN_INSTALL_ROOT "")
if (EXISTS "${GLEAN_INSTALL_ROOT}")
    message("using glean dependencies")
    message("installing to ${GLEAN_INSTALL_ROOT}")

    add_custom_target(dependency_stub)
    include_directories(SYSTEM "${GLEAN_INSTALL_ROOT}/include")
    link_directories("${GLEAN_INSTALL_ROOT}/lib")
elseif (EXISTS "${GLEAN_CACHE}")
    message("using glean dependencies")
    add_custom_target(dependency_stub)
    include_directories(SYSTEM "${GLEAN_CACHE}/include")
    link_directories("${GLEAN_CACHE}/lib")
elseif (EXISTS "${GLEAN_CACHE2}")
    message("looks like glean is in use but missing ${GLEAN_CACHE}")
elseif (EXISTS "${CMAKE_SOURCE_DIR}/glean.cmake")
    message("glean.cmake detected")
    include("${CMAKE_SOURCE_DIR}/glean.cmake")
    add_library(dependency_stub STATIC "${CMAKE_SOURCE_DIR}/dependent_projects/stub.cpp")
    add_dependencies(dependency_stub ${DEP_PROJECT_DEPS})
    include_directories(SYSTEM "${CMAKE_BINARY_DIR}/install/include")
    link_directories("${CMAKE_BINARY_DIR}/install/lib")
elseif (EXISTS "${DEP_ROOT_DIR}")
    message("${DEP_ROOT_DIR}")
    message("using dependency folder")
    add_custom_target(dependency_stub)
    include_directories(SYSTEM "${DEP_ROOT_DIR}/install/include")
    link_directories("${DEP_ROOT_DIR}/install/lib")
else ()
    message("downloading and building deps")
    include("${CMAKE_SOURCE_DIR}/dependent_projects/dependencies.txt")
    add_library(dependency_stub STATIC "${CMAKE_SOURCE_DIR}/dependent_projects/stub.cpp")
    add_dependencies(dependency_stub ${DEP_PROJECT_DEPS})
endif ()

set(HEADER_FOLDER "include")
set(TEST_FOLDER "tests")
set(SOURCE_FOLDER "src")

include_directories( ${HEADER_FOLDER} )

set( HEADER_FILES
		${HEADER_FOLDER}/daw/iso8601/daw_common.h
		${HEADER_FOLDER}/daw/iso8601/daw_date_formatting.h
		${HEADER_FOLDER}/daw/iso8601/daw_date_parsing.h
)

set( SOURCE_FILES
)

add_custom_target( full )

add_executable(iso8601_test EXCLUDE_FROM_ALL ${HEADER_FILES} ${TEST_FOLDER}/iso8601_tests.cpp )
add_dependencies( iso8601_test dependency_stub ) 
add_dependencies( full iso8601_test )

add_executable(benchmarks EXCLUDE_FROM_ALL ${HEADER_FILES} ${TEST_FOLDER}/benchmarks.cpp )
add_dependencies( benchmarks dependency_stub ) 
target_link_libraries( benchmarks ${COMPILER_SPECIFIC_LIBS} ${CMAKE_THREAD_LIBS_INIT} )
add_dependencies( full benchmarks )

add_executable(small_test EXCLUDE_FROM_ALL ${HEADER_FILES} ${TEST_FOLDER}/small_test.cpp )
add_dependencies( small_test dependency_stub ) 
add_dependencies( full small_test )

install( DIRECTORY ${HEADER_FOLDER}/daw DESTINATION include )

