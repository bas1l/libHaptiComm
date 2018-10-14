cmake_minimum_required(VERSION 2.8)

project(HaptiCommConfig)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_executable(${PROJECT_NAME} "haptiCommConfig.cpp")

link_directories(${CMAKE_BINARY_DIR})

target_link_libraries (${PROJECT_NAME} LINK_PUBLIC config4HaptiComm pthread ncurses libad5383 libutils libalphabet libwaveform)

