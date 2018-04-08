cmake_minimum_required(VERSION 3.9)
project(iso8601_parsing)

set(CMAKE_CXX_STANDARD 17)

find_package( Boost 1.58.0 COMPONENTS system iostreams filesystem regex unit_test_framework REQUIRED )
include_directories( SYSTEM ${Boost_INCLUDE_DIRS} )
link_directories( ${Boost_LIBRARY_DIRS} )

include_directories( SYSTEM "${CMAKE_SOURCE_DIR}/dependent_projects/build/install/include" )
link_directories( "${CMAKE_SOURCE_DIR}/dependent_projects/build/install/lib" )

set( HEADER_FOLDER "include" )
set( SOURCE_FOLDER "src" )
set( TEST_FOLDER "tests" )

include_directories( ${HEADER_FOLDER} )

set( HEADER_FILES
		${HEADER_FOLDER}/common.h
		${HEADER_FOLDER}/date_formatting.h
		${HEADER_FOLDER}/iso8601_timestamps.h
)

set( SOURCE_FILES
)

add_executable(iso8601_test ${HEADER_FILES} ${TEST_FOLDER}/iso8601_tests.cpp )
add_executable(benchmarks ${HEADER_FILES} ${TEST_FOLDER}/benchmarks.cpp )
target_link_libraries( benchmarks ${Boost_LIBRARIES} ${COMPILER_SPECIFIC_LIBS} ${CMAKE_THREAD_LIBS_INIT} )
add_executable(small_test ${HEADER_FILES} ${TEST_FOLDER}/small_test.cpp )

