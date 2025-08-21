cmake_minimum_required(VERSION 3.10)
project(buffer-lib LANGUAGES C CXX)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Source files
file(GLOB BUFFER_SRC
    src/*.c
    src/common/*.c
)

file(GLOB BUFFER_HEADERS
    src/*.h
    src/common/*.h
)

# Compiler flags for safety
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic")

# Create static library
add_library(buffer STATIC ${BUFFER_SRC})

target_include_directories(buffer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/common>
    $<INSTALL_INTERFACE:include>
)

# Install rules
install(TARGETS buffer
    ARCHIVE DESTINATION lib
)

install(DIRECTORY src/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Usage:
# mkdir build && cd build
# cmake .. && make
# make install DESTDIR=../install
