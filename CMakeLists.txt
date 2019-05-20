cmake_minimum_required(VERSION 3.9)
project(iso8601_parsing)

find_package( Boost 1.58.0 COMPONENTS system iostreams filesystem regex unit_test_framework REQUIRED )

enable_testing( )
add_definitions( -DBOOST_ALL_NO_LIB )

find_package( Threads REQUIRED )

add_definitions( -DBOOST_TEST_DYN_LINK -DBOOST_ALL_NO_LIB -DBOOST_ALL_DYN_LINK )

include( "${CMAKE_SOURCE_DIR}/dependent_projects/CMakeListsCompiler.txt" )

set( DEP_ROOT_DIR_BASE "${CMAKE_SOURCE_DIR}/dependent_projects/build" )
if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
	set( DEP_ROOT_DIR "${DEP_ROOT_DIR_BASE}/debug" )
elseif( "${CMAKE_BUILD_TYPE}" STREQUAL "Release" )
	set( DEP_ROOT_DIR "${DEP_ROOT_DIR_BASE}/release" )
endif( )
message( "dependency root: ${DEP_ROOT_DIR}" )

message( "Building for: ${CMAKE_BUILD_TYPE}" )
if( EXISTS "${DEP_ROOT_DIR}" )
	message( "${DEP_ROOT_DIR}" )
	message( "using dependency folder" )
	add_custom_target( dependency_stub )
	include_directories( SYSTEM "${DEP_ROOT_DIR}/install/include" )
	link_directories( "${DEP_ROOT_DIR}/install/lib" )
else( )
	message( "downloading and building deps" )
	include( "${CMAKE_SOURCE_DIR}/dependent_projects/dependencies.txt" )
	add_library( dependency_stub STATIC "${CMAKE_SOURCE_DIR}/dependent_projects/stub.cpp" )
	add_dependencies( dependency_stub ${DEP_PROJECT_DEPS} )
endif( )

include_directories( "./include" )

include_directories( SYSTEM ${Boost_INCLUDE_DIRS} )
link_directories( ${Boost_LIBRARY_DIRS} )

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

install( DIRECTORY ${HEADER_FOLDER}/daw DESTINATION include )
cmake_minimum_required( VERSION 2.8.12 )

