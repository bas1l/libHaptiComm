cmake_minimum_required(VERSION 2.8)

project(subitation)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_library(candidat "Candidat.cpp")
add_library(experiment "Experiment.cpp")

add_executable(${PROJECT_NAME} "subitation.cpp")
link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} 
			LINK_PUBLIC pthread ncurses ${Boost_SYSTEM_LIBRARY} boost_filesystem
			asound  hapticomm audioFile
			cand exp)
add_dependencies(${PROJECT_NAME} hapticomm)