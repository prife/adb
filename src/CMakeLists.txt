cmake_minimum_required(VERSION 3.5.1)
project(my-adb)

set(libadb_srcs
    adb.cpp
    adb_io.cpp
    adb_listeners.cpp
    adb_trace.cpp
    adb_unique_fd.cpp
    adb_utils.cpp
    fdevent/fdevent.cpp
    fdevent/fdevent_poll.cpp
    services.cpp
    sockets.cpp
    socket_spec.cpp
    sysdeps/errno.cpp
    transport.cpp
    transport_fd.cpp
    transport_local.cpp
    transport_usb.cpp
    types.cpp
   )

set(libadb_posix_srcs 
    sysdeps_unix.cpp
    sysdeps/posix/network.cpp
   )

set(linux_srcs
    fdevent/fdevent_epoll.cpp
    client/usb_linux.cpp
   )

set(libadb_host_src
    client/auth.cpp
    #    client/usb_libusb.cpp
    client/usb_dispatch.cpp
    #    client/transport_mdns.cpp
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
#    client/fastdeploy.cpp
#    client/fastdeploycallbacks.cpp
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
if (WIN32)
    link_directories(${CMAKE_SOURCE_DIR}/lib/windows/prebuilt/usb/)
    link_directories(${CMAKE_SOURCE_DIR}/prebuilt/windows/32/)
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    link_directories(${CMAKE_SOURCE_DIR}/prebuilt/osx)
else()
    link_directories(${CMAKE_SOURCE_DIR}/lib/boringssl/debian/out)
endif()

add_executable(${PROJECT_NAME} ${SOURCES_BASE})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/libutils/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/base/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/libbuildversion/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/diagnose_usb/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/libcrypto_utils/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/boringssl/src/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/libcutils/include
)

if (WIN32)
    target_include_directories(${PROJECT_NAME}
        PRIVATE ${CMAKE_SOURCE_DIR}/lib/windows/usb/api
    )
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE -DADB_HOST=1
	#        -DHAVE_FORKEXEC=1
	#	    -DHAVE_SYMLINKS
	#        -DHAVE_TERMIO_H
	#        -DHAVE_SYS_SOCKET_H
	#        -DHAVE_OFF64_T
    PRIVATE -D_GNU_SOURCE
	#        -D_XOPEN_SOURCE
	#        -DHAVE_ANDROID_OS=1
    PRIVATE ADB_VERSION="x.x-x"
	#        -DADB_HOST_ON_TARGET=1
    PRIVATE -DALLOW_ADBD_ROOT=1
    PRIVATE -DWORKAROUND_BUG6558362
    PRIVATE -DFAKE_LOG_DEVICE=1
    PRIVATE -DDONT_USE_LIBUSB
    PRIVATE -DDONT_USE_MDNS
    PRIVATE DONT_USE_FASTDEPLOY=1
)

if (WIN32)
    target_compile_definitions( ${PROJECT_NAME}
        PRIVATE -DUNICODE=1
        PRIVATE -D_UNICODE=1

        # -std=gnu++11 doesn't set _GNU_SOURCE on Windows.
        PRIVATE -D_GNU_SOURCE

        # MinGW hides some things behind _POSIX_SOURCE.
        PRIVATE -D_POSIX_SOURCE
   )
endif()

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

# for adbd
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
 set(libadbd_core
     ${libadb_srcs}
     ${libadb_posix_srcs}
     fdevent/fdevent_epoll.cpp
     daemon/auth.cpp
     #daemon/jdwp_service.cpp
     daemon/usb_dummy.cpp
 )

 set(libadbd_services
     daemon/file_sync_service.cpp
     daemon/services.cpp
     daemon/shell_service.cpp
     shell_service_protocol.cpp
#     daemon/abb_service.cpp
#     daemon/framebuffer_service.cpp
#     daemon/framebuffer_service.cpp
#     daemon/restart_service.cpp
 )

 add_executable(adbd
     daemon/main.cpp
     ${libadbd_core}
     ${libadbd_services}
     ../lib/adbd_auth/adbd_auth.cpp
     ../lib/libasyncio/AsyncIO.cpp
     ../lib/base/properties.cpp
     ../lib/libcutils/android_get_control_file.cpp
 )

 set_property(TARGET adbd PROPERTY CXX_STANDARD 20)

 target_compile_definitions(adbd
     PRIVATE _GNU_SOURCE
     PRIVATE ADB_VERSION="x.x-x"
 #    PRIVATE ALLOW_ADBD_ROOT=1
     PRIVATE WORKAROUND_BUG6558362
     PRIVATE FAKE_LOG_DEVICE=1
     PRIVATE DONT_USE_LIBUSB
     PRIVATE DONT_USE_MDNS
     PRIVATE ADB_NON_ANDROID=1
     PRIVATE ADB_HOST=0
     PRIVATE ALLOW_ADBD_NO_AUTH=1
 )

 target_include_directories(adbd
     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/daemon/include
     PRIVATE ${CMAKE_SOURCE_DIR}/include
     PRIVATE ${CMAKE_SOURCE_DIR}/lib/libutils/include
     PRIVATE ${CMAKE_SOURCE_DIR}/lib/adbd_auth/include
     PRIVATE ${CMAKE_SOURCE_DIR}/lib/base/include
     PRIVATE ${CMAKE_SOURCE_DIR}/lib/libbuildversion/include
     PRIVATE ${CMAKE_SOURCE_DIR}/lib/diagnose_usb/include
     PRIVATE ${CMAKE_SOURCE_DIR}/lib/libcrypto_utils/include
     PRIVATE ${CMAKE_SOURCE_DIR}/lib/boringssl/src/include
     PRIVATE ${CMAKE_SOURCE_DIR}/lib/libcutils/include
     PRIVATE ${CMAKE_SOURCE_DIR}/lib/liblog/include
     PRIVATE ${CMAKE_SOURCE_DIR}/lib/libasyncio/include
 )

 target_link_libraries(adbd ${LINK_LIBS} resolv util)

endif() # for linux
