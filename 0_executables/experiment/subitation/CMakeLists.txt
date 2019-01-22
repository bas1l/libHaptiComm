cmake_minimum_required(VERSION 2.8)

project(subitation)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_library(candidat "candidat.cpp")
add_library(experiment "experiment.cpp")

add_executable(${PROJECT_NAME} "subitation.cpp")
link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread ncurses config4HaptiComm libad5383 libutils libwaveform libdevice libalphabet  candidat experiment boost_filesystem)
