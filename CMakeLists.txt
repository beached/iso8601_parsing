cmake_minimum_required(VERSION 3.18)

project("daw-iso8601-parsing"
        VERSION "0.1.0"
        DESCRIPTION "Helpers for parsing iso8601 timestamps"
        HOMEPAGE_URL "https://github.com/beached/iso8601_parsing"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard whose features are requested.")

if (NOT DAW_USE_PACKAGE_MANAGEMENT)
    add_subdirectory(extern)
else ()
    find_package(daw-header-libraries REQUIRED)
endif ()

set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(iso8601_parsing_INSTALL_CMAKEDIR
        "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake"
        CACHE STRING
        "The install directory to use for CMake package config files")


include_directories("./include")

set(HEADER_FOLDER "include")

include_directories(${HEADER_FOLDER})

set(HEADER_FILES
        ${HEADER_FOLDER}/daw/iso8601/daw_common.h
        ${HEADER_FOLDER}/daw/iso8601/daw_date_formatting.h
        ${HEADER_FOLDER}/daw/iso8601/daw_date_parsing.h
        )

add_library(${PROJECT_NAME} INTERFACE)
add_library(daw::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} INTERFACE daw::daw-header-libraries date::date )

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
target_include_directories(${PROJECT_NAME}
        INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        )

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CMakePackageConfigHelpers)

configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION
        ${iso8601_parsing_INSTALL_CMAKEDIR})

write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

install(EXPORT ${PROJECT_NAME}_Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE daw::
        DESTINATION ${iso8601_parsing_INSTALL_CMAKEDIR})

install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${iso8601_parsing_INSTALL_CMAKEDIR})

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include/)

if (DAW_ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif ()
