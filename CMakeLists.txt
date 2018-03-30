cmake_minimum_required(VERSION 3.9)
project(iso8601_parsing)

set(CMAKE_CXX_STANDARD 14)

include_directories( SYSTEM "${CMAKE_SOURCE_DIR}/dependent_projects/build/install/include" )
link_directories( "${CMAKE_SOURCE_DIR}/dependent_projects/build/install/lib" )

set( HEADER_FOLDER "include" )
set( SOURCE_FOLDER "src" )
set( TEST_FOLDER "tests" )

include_directories( ${HEADER_FOLDER} )

set( HEADER_FILES
		${HEADER_FOLDER}/iso8601.h
)

set( SOURCE_FILES
)

add_executable(iso8601_test ${HEADER_FILES} ${TEST_FOLDER}/iso8601_tests.cpp )
