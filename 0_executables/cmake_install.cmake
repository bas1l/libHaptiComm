# Install script for directory: /home/pi/haptiComm/libHaptiComm/0_executables

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/pi/haptiComm/libHaptiComm/0_executables/determine-up/cmake_install.cmake")
  include("/home/pi/haptiComm/libHaptiComm/0_executables/demo_standard/cmake_install.cmake")
  include("/home/pi/haptiComm/libHaptiComm/0_executables/neutral/cmake_install.cmake")
  include("/home/pi/haptiComm/libHaptiComm/0_executables/caracterise2018/cmake_install.cmake")
  include("/home/pi/haptiComm/libHaptiComm/0_executables/sinefunction_generator/cmake_install.cmake")
  include("/home/pi/haptiComm/libHaptiComm/0_executables/apparent_motion_experiment/cmake_install.cmake")
  include("/home/pi/haptiComm/libHaptiComm/0_executables/tap_motion_experiment/cmake_install.cmake")
  include("/home/pi/haptiComm/libHaptiComm/0_executables/taphold_motion_experiment/cmake_install.cmake")
  include("/home/pi/haptiComm/libHaptiComm/0_executables/maximum_tap_motion_experiment/cmake_install.cmake")

endif()

