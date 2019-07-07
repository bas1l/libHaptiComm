cmake_minimum_required(VERSION 2.8)

project(haptiBraille)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
add_executable(${PROJECT_NAME} "haptiBraille.cpp")

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread ncurses hapticomm haptiBrailleDriver)
add_dependencies(${PROJECT_NAME} hapticomm)
