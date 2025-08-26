cmake_minimum_required(VERSION 3.10)
project(buffer-lib LANGUAGES C CXX)

# Set source and build directories
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build")

# Collect all source files from src and src/common
file(GLOB SRC_FILES "${SRC_DIR}/*.c")
file(GLOB COMMON_FILES "${SRC_DIR}/common/*.c")
set(SRCS ${SRC_FILES} ${COMMON_FILES})

# Set output directory for library
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILD_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILD_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Include headers
include_directories(${SRC_DIR})
include_directories(${SRC_DIR}/common)

# Compiler flags for safety
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic")

# Create static library
add_library(buffer STATIC ${SRCS})

# Set library output name to libbuffer.a
set_target_properties(buffer PROPERTIES OUTPUT_NAME "buffer")

# Install rule for the static library to local install directory
install(TARGETS buffer ARCHIVE DESTINATION ${CMAKE_SOURCE_DIR}/install/lib)

# Install headers to local install directory
install(DIRECTORY ${SRC_DIR}/ DESTINATION ${CMAKE_SOURCE_DIR}/install/include FILES_MATCHING PATTERN "*.h")

# Custom clean target to remove build, bin, and install directories
add_custom_target(extra_clean
    COMMAND rm -rf "${CMAKE_SOURCE_DIR}/build" "${CMAKE_SOURCE_DIR}/bin" "${CMAKE_SOURCE_DIR}/install"
    COMMENT "Removing build/, bin/, and install/ directories"
)

# Usage:
# mkdir build && cd build
# cmake .. && make
# make install
# By default, installs to ./install folder parallel to build.
# To override, use: cmake -DCMAKE_INSTALL_PREFIX=/your/path ..
