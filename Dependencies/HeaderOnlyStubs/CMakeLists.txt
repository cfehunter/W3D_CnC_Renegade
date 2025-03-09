cmake_minimum_required(VERSION 3.24)

add_library(headeronlystubs INTERFACE)
target_include_directories(headeronlystubs INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
