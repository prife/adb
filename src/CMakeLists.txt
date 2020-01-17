cmake_minimum_required(VERSION 3.10)
project(my-adb)

set(CMAKE_CXX_STANDARD 14)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/lib/base/include
        ${CMAKE_SOURCE_DIR}/lib/libcrypto_utils/include
        /data/workplace/wetest-adb/android-platform-external-boringssl/include
)

# FIXME
link_directories(/data/workplace/wetest-adb/android-platform-external-boringssl/debian/out)

add_definitions(
        -DADB_HOST=1
        -DHAVE_FORKEXEC=1
        -DHAVE_SYMLINKS
        -DHAVE_TERMIO_H
        -DHAVE_SYS_SOCKET_H
        -DHAVE_OFF64_T
        -D_GNU_SOURCE
        -D_XOPEN_SOURCE
        -DHAVE_ANDROID_OS=1
        -D'ADB_VERSION="x.x-x"'
        -DADB_HOST_ON_TARGET=1
        -DALLOW_ADBD_ROOT=1
        -DWORKAROUND_BUG6558362
        -DFAKE_LOG_DEVICE=1
        -DDONT_USE_LIBUSB
        -DDONT_USE_MDNS
)

add_executable(${PROJECT_NAME}
        adb.cpp
        adb_auth_host.cpp
        adb_client.cpp
        adb_io.cpp
        adb_listeners.cpp
        adb_trace.cpp
        adb_utils.cpp
        bugreport.cpp
        client/main.cpp
        client/usb_dispatch.cpp
        client/usb_linux.cpp
        commandline.cpp
        console.cpp
        diagnose_usb.cpp
        fdevent.cpp
        file_sync_client.cpp
        line_printer.cpp
        shell_service_protocol.cpp
        services.cpp
        sockets.cpp
        socket_spec.cpp
        sysdeps_unix.cpp
        sysdeps/errno.cpp
        sysdeps/posix/network.cpp
        transport.cpp
        transport_local.cpp
        transport_usb.cpp)

target_link_libraries(${PROJECT_NAME} libbase libcrypto_utils libcutils liblog crypto pthread rt)
