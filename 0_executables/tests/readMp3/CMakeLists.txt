cmake_minimum_required(VERSION 2.8)

project(readMP3)

add_executable(${PROJECT_NAME} "readMP3.cpp")

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC)
