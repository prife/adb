cmake_minimum_required(VERSION 3.5.1)
project(libbuildversion)

set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/libbuildversion/include)

add_library(${PROJECT_NAME} STATIC libbuildversion.cpp)
