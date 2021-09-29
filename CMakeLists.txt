cmake_minimum_required( VERSION 3.10 )

set(CMAKE_CXX_STANDARD 20)

project( robot_lib
        VERSION 2021.1.0
        DESCRIPTION "Cellworld vr utilities"
        LANGUAGES CXX)

find_package(Easy-tcp REQUIRED)

###
### MAIN LIBRARY SETUP
###

set(robot_lib_files
        src/robot.cpp )

add_library(robot_lib ${robot_lib_files})

set_target_properties(robot_lib
        PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED On
        CXX_EXTENSIONS Off
        VERSION ${PROJECT_VERSION})

if(MSVC)
    target_compile_options(robot_lib PRIVATE /W4)
else(MSVC)
    target_compile_options(robot_lib PRIVATE -Wall -Wextra -pedantic)
endif(MSVC)

target_include_directories(robot_lib
        SYSTEM INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_include_directories(robot_lib
        PRIVATE
        include )

target_link_libraries( robot_lib
        PUBLIC
        ${CELLWORLD_TOOLS_LIBRARIES}
        easy-tcp)

###
### TESTS
###
find_package(CatchTests CONFIG QUIET)

###
### LIBRARY INSTALLATION
###

if (CMAKE_BUILD_TYPE MATCHES Release)

    include(GNUInstallDirs)

    install(TARGETS robot_lib EXPORT Robot_libConfig
            ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})

    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT Robot_libConfig
            DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/Robot_lib
            EXPORT_LINK_INTERFACE_LIBRARIES)

    export(TARGETS robot_lib FILE Robot_libConfig.cmake)
    install(CODE "FILE(APPEND ${CMAKE_INSTALL_FULL_DATADIR}/cmake/Robot_lib/Robot_libConfig.cmake \"find_package(Easy-tcp REQUIRED)\n\") ")
    install(CODE "FILE(APPEND ${CMAKE_INSTALL_FULL_DATADIR}/cmake/Robot_lib/Robot_libConfig.cmake \"set(ROBOT_LIBRARIES robot_lib easy-tcp)\n\") ")

endif()