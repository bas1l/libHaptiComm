cmake_minimum_required(VERSION 2.8)

project(caracterise2018)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
add_executable(${PROJECT_NAME} "caracterise2018.cpp")

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread ncurses config4HaptiComm libad5383 libutils libalphabet libwaveform)
