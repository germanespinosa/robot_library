cmake_minimum_required( VERSION 3.10 )

set(CMAKE_CXX_STANDARD 20)

project( robot_lib
        VERSION 2021.1.0
        DESCRIPTION "Cellworld vr utilities"
        LANGUAGES CXX)

find_package(Dependencies)

string(APPEND CMAKE_CXX_FLAGS " -pthread ")
install_dependency(https://github.com/germanespinosa/cellworld Cellworld)
install_dependency(https://github.com/germanespinosa/params-cpp Params-cpp)
install_dependency(https://github.com/germanespinosa/tcp-messages Tcp_messages)
install_dependency(https://github.com/germanespinosa/agent_tracking Agent_tracking)


###
### MAIN LIBRARY SETUP
###

set(robot_lib_files
        src/robot.cpp
        src/robot_simulator.cpp
        src/pid_controller.cpp
        src/tracking_simulator.cpp)

add_library(robot_lib ${robot_lib_files})

add_library(gamepad_lib src/gamepad.cpp)


set_target_properties(robot_lib
        PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED On
        CXX_EXTENSIONS Off
        VERSION ${PROJECT_VERSION})

target_include_directories(robot_lib
        SYSTEM INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_include_directories(robot_lib
        PRIVATE
        include )

target_link_libraries( robot_lib
        LINK_PUBLIC
         agent_tracking
        params-cpp)

###
### UTILITIES SETUP
###

include_directories( include )

add_executable(robot src/tool/fake_robot.cpp)
target_link_libraries( robot PUBLIC robot_lib)

add_executable(stop_fake_robot src/tool/stop_fake_robot.cpp)
target_link_libraries( stop_fake_robot PUBLIC robot_lib)

add_executable(controller src/tool/controller.cpp)
target_link_libraries( controller PUBLIC robot_lib)

add_executable(gamepad src/tool/gamepad.cpp)
target_link_libraries( gamepad PUBLIC robot_lib gamepad_lib)

###
### TESTS
###
#install_dependency(https://github.com/germanespinosa/catch CatchTests)
#test_library(robot_lib)


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
    install(CODE "FILE(APPEND ${CMAKE_INSTALL_FULL_DATADIR}/cmake/Robot_lib/Robot_libConfig.cmake \"find_package(Params-cpp REQUIRED)\n\") ")
    install(CODE "FILE(APPEND ${CMAKE_INSTALL_FULL_DATADIR}/cmake/Robot_lib/Robot_libConfig.cmake \"find_package(Cellworld REQUIRED)\n\") ")
    install(CODE "FILE(APPEND ${CMAKE_INSTALL_FULL_DATADIR}/cmake/Robot_lib/Robot_libConfig.cmake \"set(ROBOT_LIBRARIES ${CELLWORLD_LIBRARIES} params-cpp )\n\") ")

endif()