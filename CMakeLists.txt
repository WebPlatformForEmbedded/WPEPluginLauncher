cmake_minimum_required(VERSION 3.3)

project(Launcher)

find_package(Thunder)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

if(BUILD_REFERENCE)
    add_definitions(-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

set(PLUGIN_LAUNCHER_STARTMODE "Activated" CACHE STRING "Automatically start the plugin")
find_package(${NAMESPACE}Plugins REQUIRED)

add_library(${MODULE_NAME} SHARED
    Launcher.cpp
    Module.cpp)

target_link_libraries(${MODULE_NAME} 
        PRIVATE 
            ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR}/${STORAGENAME}/plugins)

write_config()
