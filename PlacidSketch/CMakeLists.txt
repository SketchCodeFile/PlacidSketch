cmake_minimum_required(VERSION 3.15)
project(PlacidSketch VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find all source files
file(GLOB_RECURSE SKETCH_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Create executable
add_executable(placidsketch ${SKETCH_SOURCES})

# Include directories
target_include_directories(placidsketch PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/XSketch
)

# Compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(placidsketch PRIVATE -Wall -Wextra)
endif()

# Print configuration summary
message(STATUS "========================================")
message(STATUS "  PlacidSketch Build Configuration")
message(STATUS "========================================")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Source files:")
foreach(src ${SKETCH_SOURCES})
    message(STATUS "  ${src}")
endforeach()
message(STATUS "========================================")