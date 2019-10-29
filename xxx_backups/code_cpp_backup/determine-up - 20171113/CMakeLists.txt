cmake_minimum_required(VERSION 2.8)

project(determine-up)

add_executable(${PROJECT_NAME} "determine-up.cpp")

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread ncurses libtacom)
