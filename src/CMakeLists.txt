cmake_minimum_required(VERSION 3.5.1)
project(my-adb)

set(CMAKE_CXX_STANDARD 14)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/lib/base/include
        ${CMAKE_SOURCE_DIR}/lib/libbuildversion/include
        ${CMAKE_SOURCE_DIR}/lib/diagnose_usb/include
        ${CMAKE_SOURCE_DIR}/lib/libcrypto_utils/include
        ${CMAKE_SOURCE_DIR}/lib/boringssl/src/include
        ${CMAKE_SOURCE_DIR}/lib/libcutils/include
)

if (WIN32)
include_directories(${CMAKE_SOURCE_DIR}/lib/windows/usb/api)
link_directories(${CMAKE_SOURCE_DIR}/lib/windows/prebuilt/usb/)
link_directories(${CMAKE_SOURCE_DIR}/prebuilt/windows/32/)
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
link_directories(${CMAKE_SOURCE_DIR}/prebuilt/osx)
else()
link_directories(${CMAKE_SOURCE_DIR}/lib/boringssl/debian/out)
endif()

set(libadb_srcs
    adb.cpp
    adb_io.cpp
    adb_listeners.cpp
    adb_trace.cpp
    adb_unique_fd.cpp
    adb_utils.cpp
    fdevent.cpp
    services.cpp
    sockets.cpp
    socket_spec.cpp
    sysdeps/errno.cpp
    transport.cpp
    transport_fd.cpp
    transport_local.cpp
    transport_usb.cpp
   )


set(libadb_host_src
    client/auth.cpp
    #    client/usb_libusb.cpp
    client/usb_dispatch.cpp
    #    client/transport_mdns.cpp
    client/fastdeploy.cpp
    client/fastdeploycallbacks.cpp
   )

set(libadb_posix_srcs 
    sysdeps_unix.cpp
    sysdeps/posix/network.cpp
   )

set(linux_srcs
    client/usb_linux.cpp
   )

set(darwin_srcs
    client/usb_osx.cpp
   )

set(windows_srcs
    client/usb_windows.cpp
    sysdeps_win32.cpp
    sysdeps/win32/errno.cpp
    sysdeps/win32/stat.cpp
   )

set(adb_cmd_srcs
    client/adb_client.cpp
    client/bugreport.cpp
    client/commandline.cpp
    client/file_sync_client.cpp
    client/main.cpp
    client/console.cpp
    client/adb_install.cpp
    client/line_printer.cpp
    shell_service_protocol.cpp
   )

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  set(SOURCES_BASE
      ${libadb_srcs}
      ${adb_cmd_srcs}
      ${libadb_host_src}
      ${windows_srcs}
    )
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  set(SOURCES_BASE
      ${libadb_srcs}
      ${adb_cmd_srcs}
      ${libadb_host_src}
      ${libadb_posix_srcs}
      ${linux_srcs}
    )
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
  set(SOURCES_BASE
      ${libadb_srcs}
      ${adb_cmd_srcs}
      ${libadb_host_src}
      ${libadb_posix_srcs}
      ${darwin_srcs}
    )
else()
endif()

#message(STATUS ${SOURCES_BASE})

add_definitions(
        -DADB_HOST=1
	#        -DHAVE_FORKEXEC=1
	#	 -DHAVE_SYMLINKS
	#        -DHAVE_TERMIO_H
	#        -DHAVE_SYS_SOCKET_H
	#        -DHAVE_OFF64_T
        -D_GNU_SOURCE
	#        -D_XOPEN_SOURCE
	#        -DHAVE_ANDROID_OS=1
        -D'ADB_VERSION="x.x-x"'
	#        -DADB_HOST_ON_TARGET=1
        -DALLOW_ADBD_ROOT=1
        -DWORKAROUND_BUG6558362
        -DFAKE_LOG_DEVICE=1
        -DDONT_USE_LIBUSB
        -DDONT_USE_MDNS
)

if (WIN32)
   add_definitions(
        -DUNICODE=1
        -D_UNICODE=1

        # -std=gnu++11 doesn't set _GNU_SOURCE on Windows.
        -D_GNU_SOURCE

        # MinGW hides some things behind _POSIX_SOURCE.
        -D_POSIX_SOURCE
   )
endif()

add_executable(${PROJECT_NAME} ${SOURCES_BASE})

set(LINK_LIBS libbase libbuildversion libcrypto_utils libcutils libdiagnose_usb liblog crypto pthread)

if (WIN32)
set(LINK_LIBS ${LINK_LIBS} ssl AdbWinApi AdbWinUsbApi ws2_32 gdi32 userenv)
target_link_options(${PROJECT_NAME} PRIVATE -municode -static-libgcc -static-libstdc++)
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
set(LINK_LIBS ${LINK_LIBS} rt)
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
FIND_LIBRARY(IOKit_LIBRARY IOKit)
FIND_LIBRARY(Foundation_LIBRARY Foundation)
set(LINK_LIBS ${LINK_LIBS} ${Foundation_LIBRARY} ${IOKit_LIBRARY})
endif()

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})
