
cmake_minimum_required (VERSION 2.8)

# Raises a FATAL_ERROR if version < 2.8
cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

project (lomda CXX)

set(LOMDA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LOMDA_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

set (LOMDA_VERSION_MAJOR 1)
set (LOMDA_VERSION_MINOR 0)
set (LOMDA_VERSION_PATCH 1)

# Include directory
include_directories( include )

# Source file list
file (GLOB_RECURSE LOMDA_SOURCES
    "src/*.cpp"
)

# Executable
add_executable(${PROJECT_NAME} ${LOMDA_SOURCES})

# Installation target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Additional libraries
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/modules/")

# Build mode
IF (NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
ENDIF (NOT CMAKE_BUILD_TYPE)

# Required flags
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -pedantic -std=c++11 -lreadline")

message("Generated Makefile for '${CMAKE_BUILD_TYPE}' mode")

