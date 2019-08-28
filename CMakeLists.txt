cmake_minimum_required(VERSION 3.14)
project(usbip_libusb C)

set(CMAKE_C_STANDARD 11)

find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATH_SUFFIXES "include" "libusb" "libusb-1.0")
find_library(LIBUSB_LIBRARY NAMES usb-1.0 PATH_SUFFIXES "lib" "lib32" "lib64")

include_directories(include libsrc)

add_definitions(-DDEBUG -DUSBIP_WITH_LIBUSB -DHAVE_CONFIG_H)

add_executable(${PROJECT_NAME}
        include/os_specific/darwin/usbip_os.h
        include/linux/usbip.h
        include/linux/usbip_api.h
        include/linux/usbip_ux.h
        include/os_specific/win32/usbip_os.h
        include/os_specific/win32/usbip_sock.h
        include/usbip_os.h
        include/usbip_sock.h
        libsrc/dummy_device_driver.c
        libsrc/list.h
        libsrc/names.c
        libsrc/names.h
        libsrc/sysfs_utils.c
        libsrc/sysfs_utils.h
        libsrc/usbip_common.c
        libsrc/usbip_common.h
        libsrc/usbip_config.h
        libsrc/usbip_device_driver.h
        libsrc/usbip_host_api.c
        libsrc/usbip_host_common.h
        libsrc/usbip_host_driver.h
        libsrc/usbip_ux.c
        libsrc/usbip_ux.h
        src/usbip_network.c
        src/usbip_network.h
        src/usbipd_dev.c
        src/usbipd.c
        src/usbipd.h
        stub/stub.h
        stub/stub_common.c
        stub/stub_common.h
        stub/stub_dev.c
        stub/stub_event.c
        stub/stub_main.c
        stub/stub_rx.c
        stub/stub_tx.c)
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUSB_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBUSB_LIBRARY} pthread)