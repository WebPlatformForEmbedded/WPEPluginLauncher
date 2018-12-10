cmake_minimum_required(VERSION 3.7)

include(${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/cmake/tools.cmake)

set(PLUGIN_NAME Launcher)

message("Setting up ${PLUGIN_NAME}")

# This is a plugin so we need the Plugin configuration section.
find_package(Plugins REQUIRED)

set(PLUGIN_SOURCES
    Launcher.cpp
    Module.cpp)

# How, using regular CMake (PLUGIN_SUPPORT_DEFINITIONS) can we tell fuck CMake to inherit this.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Library definition section
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
include_directories(${PLUGIN_SUPPORT_INCLUDE_DIR})
target_link_libraries(${MODULE_NAME} ${PLUGIN_SUPPORT_LIBRARIES})

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME} ${PLUGIN_NAME})
