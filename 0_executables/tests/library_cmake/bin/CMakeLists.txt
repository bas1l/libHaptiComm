cmake_minimum_required(VERSION 2.8.7)

include_directories(${CMAKE_SOURCE_DIR}/headers)
set(SOURCES_DIR ${CMAKE_SOURCE_DIR}/sources)


add_library(b ${SOURCES_DIR}/b.cpp)
add_library(a ${SOURCES_DIR}/a.cpp)

add_library(audioFile ${SOURCES_DIR}/AudioFile.cpp)
add_library(utils ${SOURCES_DIR}/utils.cpp)

add_library(config4cpp STATIC IMPORTED)
set_target_properties(config4cpp PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libconfig4cpp.a)

add_library(ad5383 ${SOURCES_DIR}/ad5383.cpp)
add_library(device ${SOURCES_DIR}/device.cpp)
add_library(alphabet ${SOURCES_DIR}/alphabet.cpp)
add_library(waveform ${SOURCES_DIR}/waveform.cpp)
add_library(HaptiCommConfiguration ${SOURCES_DIR}/HaptiCommConfiguration.cpp)

set(C_LIB ${CMAKE_BINARY_DIR}/libcombi.a)
add_custom_target(combined
        COMMAND ar -x $<TARGET_FILE:a>
        COMMAND ar -x $<TARGET_FILE:b>
        COMMAND ar -qcs ${C_LIB} *.o
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS a b
        )
add_library(c STATIC IMPORTED GLOBAL)
add_dependencies(c combined)
set_target_properties(c
        PROPERTIES
        IMPORTED_LOCATION ${C_LIB}
)

set(HAPTICOMM_LIB ${CMAKE_BINARY_DIR}/libhapticomm.a)
add_custom_target(combined2
        COMMAND ar -x $<TARGET_FILE:utils>
        COMMAND ar -x $<TARGET_FILE:config4cpp>
        COMMAND ar -x $<TARGET_FILE:ad5383>
        COMMAND ar -x $<TARGET_FILE:device>
        COMMAND ar -x $<TARGET_FILE:alphabet>
        COMMAND ar -x $<TARGET_FILE:waveform>
        COMMAND ar -x $<TARGET_FILE:HaptiCommConfiguration>
        COMMAND ar -qcs ${HAPTICOMM_LIB} *.o
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS utils ad5383 device alphabet waveform HaptiCommConfiguration
)
add_library(hapticomm STATIC IMPORTED GLOBAL)
add_dependencies(hapticomm combined2)
set_target_properties(hapticomm
        PROPERTIES
        IMPORTED_LOCATION ${HAPTICOMM_LIB}
)



add_subdirectory("executables")
