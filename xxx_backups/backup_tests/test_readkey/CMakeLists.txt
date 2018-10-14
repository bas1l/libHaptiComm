cmake_minimum_required(VERSION 2.8)

project(test_readkey)

add_executable(${PROJECT_NAME} "main.cpp")

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread libtacom)
