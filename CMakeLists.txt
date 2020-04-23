cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")

project(n-adb)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-g)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/base)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/libcrypto_utils)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/libcutils)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/liblog)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/libbuildversion)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/diagnose_usb)
add_subdirectory(src)
