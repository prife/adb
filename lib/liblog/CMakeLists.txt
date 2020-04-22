cmake_minimum_required(VERSION 3.5.1)
project(liblog)

set(CMAKE_CXX_STANDARD 17)

#include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/base/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/liblog/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/libcutils/include)

add_definitions(
        -DLIBLOG_LOG_TAG=1006
        -DSNET_EVENT_LOG_TAG=1397638484
        -DFAKE_LOG_DEVICE=1
)

add_library(${PROJECT_NAME} STATIC
    logger_name.cpp
    logger_read.cpp
    logger_write.cpp
    logprint.cpp
    logd_writer.cpp
    log_event_write.cpp)
