cmake_minimum_required(VERSION 2.8)

project(worldhaptics_demo)

add_executable(${PROJECT_NAME} "whc_demo.cpp")

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread libtacom ncurses)
