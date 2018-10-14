cmake_minimum_required(VERSION 2.8)

project(test_vectors)

add_executable(${PROJECT_NAME} "test_vectors.cpp")

link_directories(${CMAKE_BINARY_DIR})
target_link_libraries (${PROJECT_NAME} LINK_PUBLIC pthread ncurses libad5383 libutils libalphabet libwaveform)
