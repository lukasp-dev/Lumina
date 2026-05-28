cmake_minimum_required(VERSION 3.20)
project(Lumina LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
add_compile_options(-Wall -Wextra -Wpedantic)

# Find liburing
find_library(URING_LIB uring REQUIRED)

# Main executable
add_executable(lumina src/main.cpp)
target_include_directories(lumina PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(lumina PRIVATE ${URING_LIB})
