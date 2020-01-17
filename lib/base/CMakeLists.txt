cmake_minimum_required(VERSION 3.10)
project(libbase)

set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/base/include)

add_library(${PROJECT_NAME} STATIC
        file.cpp
        stringprintf.cpp
        strings.cpp
        logging.cpp
        parsenetaddress.cpp
        threads.cpp
        test_utils.cpp)
