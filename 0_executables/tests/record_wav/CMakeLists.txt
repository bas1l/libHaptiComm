cmake_minimum_required(VERSION 2.8)

project(record_wav)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_executable(${PROJECT_NAME} "main.cpp")
link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread ncurses ${Boost_SYSTEM_LIBRARY} boost_filesystem pocketsphinx sphinxbase sphinxad)
