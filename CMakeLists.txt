cmake_minimum_required(VERSION 3.9)
project(iso8601_parsing)

set(CMAKE_CXX_STANDARD 17)

find_package( Boost 1.58.0 COMPONENTS system iostreams filesystem regex unit_test_framework REQUIRED )
include_directories( SYSTEM ${Boost_INCLUDE_DIRS} )
link_directories( ${Boost_LIBRARY_DIRS} )

if( EXISTS "${CMAKE_SOURCE_DIR}/dependent_projects/build" )
	add_custom_target( dependency_stub )
	include_directories( SYSTEM "${CMAKE_SOURCE_DIR}/dependent_projects/build/install/include" )
	link_directories( "${CMAKE_SOURCE_DIR}/dependent_projects/build/install/lib" )
else( )
	set( SHOULD_BUILD_DEPS ON )
	include( "${CMAKE_SOURCE_DIR}/dependent_projects/dependencies.txt" )

	add_library( dependency_stub STATIC "${CMAKE_SOURCE_DIR}/dependent_projects/stub.cpp" )
	add_dependencies( dependency_stub header_libraries_prj date_prj )
endif( )

set( HEADER_FOLDER "include" )
set( SOURCE_FOLDER "src" )
set( TEST_FOLDER "tests" )

include_directories( ${HEADER_FOLDER} )

set( HEADER_FILES
		${HEADER_FOLDER}/daw/iso8601/daw_common.h
		${HEADER_FOLDER}/daw/iso8601/daw_date_formatting.h
		${HEADER_FOLDER}/daw/iso8601/daw_date_parsing.h
)

set( SOURCE_FILES
)

add_executable(iso8601_test ${HEADER_FILES} ${TEST_FOLDER}/iso8601_tests.cpp )
add_dependencies( iso8601_test dependency_stub ) 

add_executable(benchmarks ${HEADER_FILES} ${TEST_FOLDER}/benchmarks.cpp )
add_dependencies( benchmarks dependency_stub ) 
target_link_libraries( benchmarks ${Boost_LIBRARIES} ${COMPILER_SPECIFIC_LIBS} ${CMAKE_THREAD_LIBS_INIT} )

add_executable(small_test ${HEADER_FILES} ${TEST_FOLDER}/small_test.cpp )
add_dependencies( small_test dependency_stub ) 

install( DIRECTORY ${HEADER_FOLDER}/daw/iso8601/ DESTINATION include/daw/iso8601/ )
