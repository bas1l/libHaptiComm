cmake_minimum_required(VERSION 2.8)

project(recursivecopy)

add_executable(${PROJECT_NAME} "recursive_copy.cpp")

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC)
