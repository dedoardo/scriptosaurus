cmake_minimum_required(VERSION 3.1)
project(Scriptosaurus)

option(SHARED_LIB "Build as shared library / DLL")

# Setting compiler flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

# Warnings and C++ standard
set(CMAKE_CXX_FLAGS "-Wall -pedantic -std=c++11")

# Debug infos
set(CMAKE_CXX_FLAGS_DEBUG -g)
set(CMAKE_CXX_FLAGS_RELEASE -02)

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

# Warnings and C++ standard
set(CMAKE_CXX_FLAGS "/EHsc /W4")

# Debug infos
set(CMAKE_CXX_FLAGS_DEBUG /Zi)
set(CMAKE_CXX_FLAGS_RELEASE /02)

# Removes some warnings
add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)

endif()

# Builds as static as default
if (SHARED_LIB)
	add_definitions(-DSCRIPTOSAURUS_BUILD_SHARED=1)
endif(SHARED_LIB)

# Adding include directories
include_directories("include")

# Adding source files
set(SOURCE_FILES src/script.cpp src/script_manager.cpp include/ioutput.hpp include/script.hpp include/script_manager.hpp)

# Set output directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/samples/bin)

# Add include directories
include_directories(include)

# Compile
add_library(Scriptosaurus ${SOURCE_FILES})

# Compiling example too
add_executable(Scriptosaurus_Sample1 samples/sample1.cpp)
target_link_libraries(Scriptosaurus_Sample1 ws2_32)