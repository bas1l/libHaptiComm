cmake_minimum_required(VERSION 2.8.7)

include_directories("headers")
include_directories("sources")
add_library(b b.cpp b.h)
add_library(a a.cpp a.h)

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

add_executable(main main.cpp)
target_link_libraries(main c)