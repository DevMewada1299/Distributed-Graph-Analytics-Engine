cmake_minimum_required(VERSION 3.14)
project(DistributedGraphEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Try to find MPI
find_package(MPI QUIET)

if(MPI_FOUND)
    message(STATUS "MPI found. Building distributed version.")
    add_definitions(-DUSE_MPI)
else()
    message(STATUS "MPI NOT found. Building single-node mock version.")
endif()

# Find OpenMP
find_package(OpenMP QUIET)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found.")
else()
    message(STATUS "OpenMP NOT found. Building serial version.")
endif()

# Gather sources
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create executable
add_executable(dgraph_engine ${SOURCES})

# Link libraries
if(MPI_FOUND)
    target_link_libraries(dgraph_engine PRIVATE MPI::MPI_CXX)
endif()

if(OpenMP_CXX_FOUND)
    target_link_libraries(dgraph_engine PRIVATE OpenMP::OpenMP_CXX)
endif()

# Compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dgraph_engine PRIVATE -O3 -Wall -Wextra)
endif()
