cmake_minimum_required(VERSION 3.14)
project(usbip_libusb C)

set(CMAKE_C_STANDARD 11)

find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATH_SUFFIXES "include" "libusb" "libusb-1.0")
find_library(LIBUSB_LIBRARY NAMES usb-1.0 PATH_SUFFIXES "lib" "lib32" "lib64")

include_directories(include libsrc)

add_definitions(-DDEBUG -DUSBIP_WITH_LIBUSB -DHAVE_CONFIG_H)

add_executable(${PROJECT_NAME}
        include/linux/usbip.h
        include/linux/usbip_api.h
        include/list.h
        libsrc/names.c
        libsrc/names.h
        libsrc/sysfs_utils.c
        libsrc/sysfs_utils.h
        libsrc/usbip_common.c
        libsrc/usbip_common.h
        libsrc/usbip_config.h
        src/usbip_network.c
        src/usbip_network.h
        src/usbipd_dev.c
        src/usbipd.c
        src/usbipd.h
        driver-libusb/stub.h
        driver-libusb/stub_common.c
        driver-libusb/stub_common.h
        driver-libusb/usbip_host_driver.c
        driver-libusb/stub_event.c
        driver-libusb/stub_main.c
        driver-libusb/stub_rx.c
        driver-libusb/stub_tx.c include/usbip_host_driver.h)
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUSB_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBUSB_LIBRARY} pthread)