cmake_minimum_required(VERSION 2.8)

project(record_alsa)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

#add_library(expAlsa "Experiment.cpp")
#add_library(candAlsa "Candidat.cpp")

add_executable(${PROJECT_NAME} "main.cpp")
link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC asound audioFile)
#target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread ncurses audioFile config4HaptiComm libad5383 libutils libwaveform libdevice libalphabet ${Boost_SYSTEM_LIBRARY} boost_filesystem pocketsphinx sphinxbase sphinxad)
