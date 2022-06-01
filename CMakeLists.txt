cmake_minimum_required(VERSION 3.2)
project(ELSAR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast -g -march=native")
set(CMAKE_BUILD_TYPE RELEASE)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# OpenMP library
find_package(OpenMP)

# Learned Sort library
include_directories(${PROJECT_SOURCE_DIR}/include)

# Executable
set(BINARY ${CMAKE_PROJECT_NAME})
add_executable(${BINARY} main.cc)

# Link OMP
if(OpenMP_CXX_FOUND)
    target_link_libraries(${BINARY} PRIVATE OpenMP::OpenMP_CXX)
endif()

# Filesystem library
target_link_libraries(${BINARY} PRIVATE stdc++fs)