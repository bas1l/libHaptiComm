# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.7

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/pi/haptiComm/libHaptiComm

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/pi/haptiComm/libHaptiComm

# Include any dependencies generated for this target.
include 0_executables/determine-up/CMakeFiles/determine-up.dir/depend.make

# Include the progress variables for this target.
include 0_executables/determine-up/CMakeFiles/determine-up.dir/progress.make

# Include the compile flags for this target's objects.
include 0_executables/determine-up/CMakeFiles/determine-up.dir/flags.make

0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o: 0_executables/determine-up/CMakeFiles/determine-up.dir/flags.make
0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o: 0_executables/determine-up/determine-up.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/haptiComm/libHaptiComm/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object 0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o"
	cd /home/pi/haptiComm/libHaptiComm/0_executables/determine-up && /usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/determine-up.dir/determine-up.cpp.o -c /home/pi/haptiComm/libHaptiComm/0_executables/determine-up/determine-up.cpp

0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/determine-up.dir/determine-up.cpp.i"
	cd /home/pi/haptiComm/libHaptiComm/0_executables/determine-up && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pi/haptiComm/libHaptiComm/0_executables/determine-up/determine-up.cpp > CMakeFiles/determine-up.dir/determine-up.cpp.i

0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/determine-up.dir/determine-up.cpp.s"
	cd /home/pi/haptiComm/libHaptiComm/0_executables/determine-up && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pi/haptiComm/libHaptiComm/0_executables/determine-up/determine-up.cpp -o CMakeFiles/determine-up.dir/determine-up.cpp.s

0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o.requires:

.PHONY : 0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o.requires

0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o.provides: 0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o.requires
	$(MAKE) -f 0_executables/determine-up/CMakeFiles/determine-up.dir/build.make 0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o.provides.build
.PHONY : 0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o.provides

0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o.provides.build: 0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o


# Object files for target determine-up
determine__up_OBJECTS = \
"CMakeFiles/determine-up.dir/determine-up.cpp.o"

# External object files for target determine-up
determine__up_EXTERNAL_OBJECTS =

0_executables/determine-up/determine-up: 0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o
0_executables/determine-up/determine-up: 0_executables/determine-up/CMakeFiles/determine-up.dir/build.make
0_executables/determine-up/determine-up: liblibad5383.a
0_executables/determine-up/determine-up: liblibutils.a
0_executables/determine-up/determine-up: liblibalphabet.a
0_executables/determine-up/determine-up: liblibwaveform.a
0_executables/determine-up/determine-up: liblibutils.a
0_executables/determine-up/determine-up: libaudioFile.a
0_executables/determine-up/determine-up: liblibdevice.a
0_executables/determine-up/determine-up: 0_executables/determine-up/CMakeFiles/determine-up.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pi/haptiComm/libHaptiComm/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable determine-up"
	cd /home/pi/haptiComm/libHaptiComm/0_executables/determine-up && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/determine-up.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
0_executables/determine-up/CMakeFiles/determine-up.dir/build: 0_executables/determine-up/determine-up

.PHONY : 0_executables/determine-up/CMakeFiles/determine-up.dir/build

0_executables/determine-up/CMakeFiles/determine-up.dir/requires: 0_executables/determine-up/CMakeFiles/determine-up.dir/determine-up.cpp.o.requires

.PHONY : 0_executables/determine-up/CMakeFiles/determine-up.dir/requires

0_executables/determine-up/CMakeFiles/determine-up.dir/clean:
	cd /home/pi/haptiComm/libHaptiComm/0_executables/determine-up && $(CMAKE_COMMAND) -P CMakeFiles/determine-up.dir/cmake_clean.cmake
.PHONY : 0_executables/determine-up/CMakeFiles/determine-up.dir/clean

0_executables/determine-up/CMakeFiles/determine-up.dir/depend:
	cd /home/pi/haptiComm/libHaptiComm && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pi/haptiComm/libHaptiComm /home/pi/haptiComm/libHaptiComm/0_executables/determine-up /home/pi/haptiComm/libHaptiComm /home/pi/haptiComm/libHaptiComm/0_executables/determine-up /home/pi/haptiComm/libHaptiComm/0_executables/determine-up/CMakeFiles/determine-up.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : 0_executables/determine-up/CMakeFiles/determine-up.dir/depend

