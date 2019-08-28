VisualStudio solution structure example

[For usbip and usbipd]

$(SolutionDir)
|
+-getopt
|   Type:    dll
|   Sources: getopt.c
|   Headers: getopt.h
|   NOTE: these files are excluded. Use GNU or compatible open source.
|
+-lib
|   Type:    lib
|   Sources: usbip_network.c usbip_common.c usbip_host_api.c names.c
|            stub_*.c # from libusb/stub
|            dummy_device_driver.c # from libusb/libsrc
|   Headers: usbip_network.h usbip_common.h usbip_config.h
|            usbip_host_common.h usbip_host_driver.h usbip_device_driver.h
|            names.h list.h
|            stub_common.h stub.h # from libusb/stub
|            usbip_os.h usbip_sock.h # from libusb/os
|   Includes: $(SolutionDir);$(SolutionDir)\win32
|   Definitions: USBIP_WITH_LIBUSB;HAVE_CONFIG_H
|
+-libusb-1.0 (dll)
|   Added as existing project from <libusb-src>\msvc\libusb_dll_<ver>.vcxproj
|   OutputDir: $(SolutionDir)$(Configuration)\
|
+-usbip
|   Type:    exe
|   Sources: usbip.c usbip_bind.c usbip_connect.c usbip_disconnect.c
|            usbip_list.c usbip_unbind.c
|   Headers: usbip.h
|   Includes: $(SolutionDir);$(SolutionDir)\lib;$(SolutionDir)\getopt;
|            $(SolutionDir)\win32
|   Definitions: USBIP_WITH_LIBUSB;HAVE_CONFIG_H;USBIDS_FILE="..\\usb.ids"
|   Depending Files: ws2_32.lib
|   Reference: getopt;lib;libusb-1.0 (dll);win32
|
+-usbipd
|   Type:    exe
|   Sources: usbipd.c usbipd_dev.c
|   Headers: usbipd.h
|   Includes: $(SolutionDir);$(SolutionDir)\lib;$(SolutionDir)\getopt;
|            $(SolutionDir)\win32
|   Definitions: USBIP_WITH_LIBUSB;HAVE_CONFIG_H
|   Depending Files: ws2_32.lib
|   Reference: getopt;lib;libusb-1.0 (dll);win32
|
+-win32
|   Headers: usbip_os.h usbip_sock.h # from libusb/os/win32
|
+linux
|   Headers: usbip_api.h # from include/uapi/linux
|
+-config.h
|   Copied from libusb/os/win32
|
+-usb.ids
|   Copied from /usr/share/hwdata/ or downloaded from net.
|   AccentAcute character must be modified to avoid invalid ascii exception.
|   ex) sed -i -e 's/\xb4/\x00/' usb.ids
|
+-libusb-1.0
    SYMLINK to <libusb-src>\libusb
    ex) mklink /D libusb-1.0 <libusb-src>\libusb

NOTE:
    To support Windows 7 and Visual Studio 2012, programs must not be c99.

    Winsock headers must be included at first than any other headers.
    Please, check the position which includes usbip_sock.h if duplicated
    definition errors relating  winsock headers occur.
