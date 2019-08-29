cmake_minimum_required(VERSION 3.2)
project(usbip_libusb C)

set(CMAKE_C_STANDARD 11)

find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATH_SUFFIXES "include" "libusb" "libusb-1.0")
find_library(LIBUSB_LIBRARY NAMES usb-1.0 PATH_SUFFIXES "lib" "lib32" "lib64")
if(NOT LIBUSB_INCLUDE_DIR OR NOT LIBUSB_LIBRARY)
    error("libusb not found")
endif()

include_directories(include libsrc)

add_definitions(-DDEBUG)

add_executable(${PROJECT_NAME}
        include/usbip.h
        include/list.h
        src/names.c
        src/names.h
        src/usbip_common.c
        include/usbip_common.h
        src/usbip_network.c
        src/usbip_network.h
        src/usbipd_dev.c
        src/usbipd.c
        src/usbipd_dev.h
        driver-libusb/stub.h
        driver-libusb/stub_common.c
        driver-libusb/stub_common.h
        driver-libusb/usbip_host_driver.c
        driver-libusb/stub_event.c
        driver-libusb/stub_main.c
        driver-libusb/stub_rx.c
        driver-libusb/stub_tx.c include/usbip_host_driver.h include/usbip_debug.h src/usbip_debug.c)
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUSB_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBUSB_LIBRARY} pthread)