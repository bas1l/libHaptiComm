cmake_minimum_required(VERSION 2.8)

project(maximumTapmotionExperiment)


set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_executable(${PROJECT_NAME} "maximum_tapmotion_experiment.cpp")

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread ncurses config4HaptiComm libad5383 libutils libwaveform libdevice libalphabet)
