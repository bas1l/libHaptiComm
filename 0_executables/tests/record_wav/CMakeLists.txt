cmake_minimum_required(VERSION 2.8)

project(record_wav)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_library(exp "Experiment.cpp")
add_library(cand "Candidat.cpp")

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries (${PROJECT_NAME} 
			LINK_PUBLIC pthread ncurses ${Boost_SYSTEM_LIBRARY} boost_filesystem
			asound  hapticomm audioFile
			cand exp)
add_dependencies(${PROJECT_NAME} hapticomm)