cmake_minimum_required(VERSION 3.5.1)
project(libcutils)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/libcutils/include)

add_definitions(
        -D_POSIX_C_SOURCE=200112L
)

if (WIN32)
add_library(${PROJECT_NAME} STATIC
        socket_inaddr_any_server_windows.c
	#socket_local_client_unix.c
        #socket_local_server_unix.c
        socket_network_client_windows.c
        sockets_windows.cpp
        load_file.c
        threads.c
        sockets.cpp
        )
else()
add_library(${PROJECT_NAME} STATIC
        socket_inaddr_any_server_unix.c
        socket_local_client_unix.c
        socket_local_server_unix.c
        socket_network_client_unix.c
        load_file.c
        threads.c
#        sockets_unix.cpp
        sockets.cpp)
endif()
