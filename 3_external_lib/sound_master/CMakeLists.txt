cmake_minimum_required(VERSION 2.8)

project(soundMaster)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

#include_directories("include")
add_library(wavFileReader "src/wav_file.cpp")



