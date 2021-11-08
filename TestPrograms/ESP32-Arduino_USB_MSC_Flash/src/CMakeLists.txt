# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.
cmake_minimum_required(VERSION 3.5)


FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
FILE(GLOB_RECURSE lib_sources ${CMAKE_SOURCE_DIR}/lib/*.*)

idf_component_register(SRCS ${app_sources} INCLUDE_DIRS . ${COMPONENT_DIR} ${lib_sources})
project(ESP32-Arduino_USB_MSC)
