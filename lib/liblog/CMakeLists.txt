cmake_minimum_required(VERSION 3.5.1)
project(liblog)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/base/include)

add_definitions(
        -DLIBLOG_LOG_TAG=1005
        -DFAKE_LOG_DEVICE=1
        -D_POSIX_C_SOURCE=199309L
)

add_library(${PROJECT_NAME} STATIC
        logd_write.c
        log_event_write.c
        fake_log_device.c)
