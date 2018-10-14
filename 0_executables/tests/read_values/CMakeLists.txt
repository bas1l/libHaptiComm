cmake_minimum_required(VERSION 2.8)

project(readvalues)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
add_executable(${PROJECT_NAME} "read_values.cpp")

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread ncurses libad5383 libutils libalphabet libwaveform libGPIOClass)
