cmake_minimum_required(VERSION 4.2)

project(voxel-adventure VERSION 0.0.0 LANGUAGES CXX)

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

add_subdirectory(vendor)
add_subdirectory(libs)
add_subdirectory(editor)
