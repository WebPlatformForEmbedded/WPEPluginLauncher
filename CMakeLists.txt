cmake_minimum_required(VERSION 3.7)

include(${CMAKE_SYSROOT}/usr/include/cmake/WPEFramework.cmake)

set(PLUGIN_NAME Launcher)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

message("Setting up ${PLUGIN_NAME}")

# This is a plugin so we need the Plugin configuration section.
find_package(Plugins REQUIRED)

set(PLUGIN_SOURCES
    Launcher.cpp
    Module.cpp)

# Library definition section
add_library(${MODULE_NAME} SHARED ${PLUGIN_SOURCES})
add_compiler_flags(${MODULE_NAME} "${PLUGINS_DEFINITIONS}")
include_directories(${PLUGINS_INCLUDE_DIR})
target_link_libraries(${MODULE_NAME} ${PLUGINS_LIBRARIES})

# Library installation section
string(TOLOWER ${NAMESPACE} STORAGENAME)
install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGENAME}/plugins)

write_config(${PLUGIN_NAME})
