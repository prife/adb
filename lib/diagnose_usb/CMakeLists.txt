cmake_minimum_required(VERSION 3.5.1)
project(libdiagnose_usb)

set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/base/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/diagnose_usb/include)

add_library(${PROJECT_NAME} STATIC diagnose_usb.cpp)
