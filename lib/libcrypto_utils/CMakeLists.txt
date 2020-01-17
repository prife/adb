cmake_minimum_required(VERSION 3.10)
project(libcrypto_utils)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/libcrypto_utils/include)
include_directories(/data/workplace/wetest-adb/android-platform-external-boringssl/include)


add_library(${PROJECT_NAME} STATIC android_pubkey.c)
