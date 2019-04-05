cmake_minimum_required(VERSION 2.8)

project(libtacom)

set(CMAKE_CXX_FLAGS "-O2 -std=c++11")


# A. Include directory
include_directories("1_include")
include_directories("/usr/local/include/sphinxbase")
include_directories("/usr/local/include/pocketsphinx")


# B. External libraries
## B.1  Dynamic libraries
add_library(wavFileReader 	"2_src/wav_file.cpp")
add_library(audioFile 		"3_external_lib/AudioFile-master/AudioFile.cpp")
## B.2  Static libraries
add_library(config4cpp STATIC IMPORTED)
set_target_properties(config4cpp PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/3_external_lib/libconfig4cpp.a)
find_package(Boost COMPONENTS filesystem REQUIRED)


# C. Developped libraries
add_library(libad5383 		"2_src/ad5383.cpp")
add_library(libdevice 		"2_src/device.cpp")
add_library(libGPIOClass 	"2_src/GPIOClass.cpp")
add_library(libutils		"2_src/utils.cpp")

add_library(libwaveform 	"2_src/waveform.cpp")
target_link_libraries(libwaveform libutils libdevice audioFile)

add_library(libalphabet 	"2_src/alphabet.cpp")
target_link_libraries(libalphabet libwaveform libdevice)

add_library(config4HaptiComm "2_src/HaptiCommConfiguration.cpp")
target_link_libraries(config4HaptiComm config4cpp libdevice libwaveform)


# D. C executable files directories
add_subdirectory("0_executables")
