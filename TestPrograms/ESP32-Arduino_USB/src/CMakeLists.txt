# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.
cmake_minimum_required(VERSION 3.5)


FILE(GLOB_RECURSE app_sources C:/SA-OST-2021/fleet-monitor-embedded/TestPrograms/ESP32-Arduino_USB/src/*.*)

idf_component_register(SRCS ${app_sources})
project(ESP32-Arduino_USB)
