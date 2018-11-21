cmake_minimum_required(VERSION 2.8)

project(libtacom)

set(CMAKE_CXX_FLAGS "-O2 -std=c++11")


###
# Include directory
###
include_directories("1_include")


### EXTERNAL LIBRARIES ###
# Static Libraries
add_library(config4cpp STATIC IMPORTED)
#add_library(filesystem STATIC IMPORTED)
set_target_properties(config4cpp PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/3_external_lib/libconfig4cpp.a)
#set_target_properties(filesystem PROPERTIES IMPORTED_LOCATION /usr/lib/arm-linux-gnueabihf/libboost_filesystem.a) 
# Shared Objects
#set (filesystem /usr/lib/arm-linux-gnueabihf/libboost_filesystem.so)
# Dynamic Libraries
add_library(wavFileReader "2_src/wav_file.cpp")
add_library(audioFile "3_external_lib/AudioFile-master/AudioFile.cpp")
#add_subdirectory("external_lib/sound_master")
# Boost libraries
#set(Boost_INCLUDE_DIR 3_external_lib/boost_1_69_0)
#set(Boost_LIBRARY_DIR 3_external_lib/boost_1_69_0/libs)
find_package(Boost COMPONENTS filesystem REQUIRED)
#include_directories(${Boost_INCLUDE_DIR})
#link_directories(${Boost_LIBRARY_DIR})



### OUR LIBRARIES ###
# Dynamic Libraries
add_library(libad5383 		"2_src/ad5383.cpp")
add_library(libGPIOClass 	"2_src/GPIOClass.cpp")
add_library(libutils		"2_src/utils.cpp")
add_library(libdevice 		"2_src/device.cpp")

add_library(libwaveform 	"2_src/waveform.cpp")
target_link_libraries(libwaveform libutils libdevice audioFile)

add_library(libalphabet 	"2_src/alphabet.cpp")
target_link_libraries(libalphabet libwaveform libdevice)

add_library(config4HaptiComm "2_src/HaptiCommConfiguration.cpp")
target_link_libraries(config4HaptiComm config4cpp libdevice libwaveform)


###
# Executable Directory
###
add_subdirectory("0_executables")
