cmake_minimum_required(VERSION 3.5.1)
project(adb)

# Add a custom command that produces version.cpp, plus
# a dummy output that's not actually produced, in order
# to force version.cmake to always be re-run before the build
ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/_version.cpp
    COMMAND ${CMAKE_COMMAND} -P
            ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake)
set(version_src ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)

set(CMAKE_CXX_STANDARD 14)

set(libadb_srcs
    ${version_src}
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
if (WIN32)
    link_directories(${CMAKE_SOURCE_DIR}/lib/windows/prebuilt/usb/)
    link_directories(${CMAKE_SOURCE_DIR}/prebuilt/windows/32/)
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    link_directories(${CMAKE_SOURCE_DIR}/prebuilt/osx)
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    link_directories(${CMAKE_SOURCE_DIR}/prebuilt/linux/aarch64)
  else()
    link_directories(${CMAKE_SOURCE_DIR}/prebuilt/linux/x86-64)
  endif()
endif()

add_executable(${PROJECT_NAME} ${SOURCES_BASE})

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/include
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
    set_target_properties(${PROJECT_NAME} PROPERTIES
                        LINK_FLAGS "-Wl,-rpath=/opt/lib/android/"
                        BUILD_WITH_INSTALL_RPATH 1)
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    FIND_LIBRARY(IOKit_LIBRARY IOKit)
    FIND_LIBRARY(Foundation_LIBRARY Foundation)
    set(LINK_LIBS ${LINK_LIBS} ${Foundation_LIBRARY} ${IOKit_LIBRARY})
endif()

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})

# for adbd
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64") # no support aarch64
set(libadbd_core
    ${libadb_srcs}
    ${libadb_posix_srcs}
    daemon/auth.cpp
    #daemon/jdwp_service.cpp
    daemon/usb.cpp
)

set(libadbd_services
    daemon/file_sync_service.cpp
    daemon/framebuffer_service.cpp
    #daemon/mdns.cpp
    #daemon/remount_service.cpp
    daemon/services.cpp
    #daemon/set_verity_enable_state_service.cpp
    daemon/shell_service.cpp
    shell_service_protocol.cpp
)

add_executable(adbd
    daemon/main.cpp
    ${libadbd_core}
    ${libadbd_services}
    ../lib/libasyncio/AsyncIO.cpp
    ../lib/base/properties.cpp
    ../lib/libcutils/android_get_control_file.cpp
)

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
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/base/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/libbuildversion/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/diagnose_usb/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/libcrypto_utils/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/boringssl/src/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/libcutils/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/libcutils/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/libasyncio/include
)

target_link_libraries(adbd ${LINK_LIBS} resolv util)

endif() # for x86-64

endif() # for linux
