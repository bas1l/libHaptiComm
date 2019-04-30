cmake_minimum_required(VERSION 2.8.7)

include_directories(${CMAKE_SOURCE_DIR}/headers)
link_directories(${CMAKE_BINARY_DIR})

add_executable(main main.cpp)
target_link_libraries(main c)
add_dependencies(main c)


add_executable(demo demo.cpp)
target_link_libraries (demo LINK_PUBLIC pthread ncurses hapticomm audioFile)
add_dependencies(demo hapticomm)
