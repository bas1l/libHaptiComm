cmake_minimum_required(VERSION 2.8)

project(test_adxl345)
set(CMAKE_BUILD_TYPE RelWithDebInfo)


add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries (${PROJECT_NAME}  LINK_PUBLIC pthread ncurses ${Boost_SYSTEM_LIBRARY} boost_filesystem adxl345)
add_dependencies(${PROJECT_NAME} hapticomm)