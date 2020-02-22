cmake_minimum_required(VERSION 3.5.1)
project(libbase)

set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/base/include)

set (SOURCES_BASE
        file.cpp
        logging.cpp
        parsenetaddress.cpp
        stringprintf.cpp
        strings.cpp
        threads.cpp
        test_utils.cpp
    )

if (WIN32)
set (SOURCES_BASE ${SOURCES_BASE}
      errors_windows.cpp
      utf8.cpp
    )
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCES_BASE})
