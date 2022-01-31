cmake_minimum_required(VERSION 2.8)

project(libhapticomm)

set(CMAKE_CXX_FLAGS "-O2 -std=c++11")

message("${CMAKE_SYSTEM_NAME}")

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/1_include)
set(SOURCES_DIR ${CMAKE_SOURCE_DIR}/2_src)
set(EXT_LIB_DIR ${CMAKE_SOURCE_DIR}/3_external_lib)

# A. Include directory
include_directories(${INCLUDE_DIR})

# B. External libraries
find_package(Boost COMPONENTS filesystem REQUIRED)
## B.1  Dynamic libraries
add_library(audioFile ${EXT_LIB_DIR}/AudioFile-master/AudioFile.cpp)
## B.2  Static libraries
add_library(config4cpp STATIC IMPORTED)
set_target_properties(config4cpp PROPERTIES IMPORTED_LOCATION ${EXT_LIB_DIR}/libconfig4cpp.a)


# C. Developped libraries
add_library(utils ${SOURCES_DIR}/utils.cpp)
add_library(ad5383 ${SOURCES_DIR}/ad5383.cpp)
add_library(device ${SOURCES_DIR}/device.cpp)
add_library(alphabet ${SOURCES_DIR}/alphabet.cpp)
add_library(waveform ${SOURCES_DIR}/waveform.cpp)
add_library(HaptiCommConfiguration ${SOURCES_DIR}/HaptiCommConfiguration.cpp)

set(HAPTICOMM_LIB ${CMAKE_BINARY_DIR}/libhapticomm.a)
add_custom_target(combined
        COMMAND ar -x $<TARGET_FILE:utils>
        COMMAND ar -x $<TARGET_FILE:config4cpp>
        COMMAND ar -x $<TARGET_FILE:ad5383>
        COMMAND ar -x $<TARGET_FILE:device>
        COMMAND ar -x $<TARGET_FILE:alphabet>
        COMMAND ar -x $<TARGET_FILE:waveform>
        COMMAND ar -x $<TARGET_FILE:HaptiCommConfiguration>
        COMMAND ar -x $<TARGET_FILE:audioFile>
        COMMAND ar -qcs ${HAPTICOMM_LIB} *.o
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS utils config4cpp ad5383 device alphabet waveform HaptiCommConfiguration audioFile
)
add_library(hapticomm STATIC IMPORTED GLOBAL)
add_dependencies(hapticomm combined)
set_target_properties(hapticomm
        PROPERTIES
        IMPORTED_LOCATION ${HAPTICOMM_LIB}
)

# D. C executable files directories
add_subdirectory("0_executables")
