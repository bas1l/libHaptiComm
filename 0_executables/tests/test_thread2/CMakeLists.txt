cmake_minimum_required(VERSION 2.8)

project(test_thread2)

add_library(app "Application.cpp")
add_executable(${PROJECT_NAME} "main.cpp")

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread app)
