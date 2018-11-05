cmake_minimum_required(VERSION 2.8)

project(appmotionExperiment)


set(CMAKE_BUILD_TYPE RelWithDebInfo)

include_directories("include")
add_library(tools "src/tools.cpp")

add_executable(${PROJECT_NAME} "appmotion_experiment.cpp")

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread ncurses config4HaptiComm libad5383 libutils libwaveform libdevice libalphabet tools)
