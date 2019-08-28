#!/bin/sh

DIR=`dirname $0`
CMD=`basename $0`

TOP=$DIR/../../..
INC=$TOP/../../../include
HWDATA=/usr/share/hwdata/

if [ -z "$1" ]; then
	echo "Usage: $CMD <distination-dir>"
	exit 1
fi

DST_SOL=$1

DST_LIB=$DST_SOL/lib
DST_USBIP=$DST_SOL/usbip
DST_USBIPD=$DST_SOL/usbipd
DST_WIN=$DST_SOL/win32
DST_LINUX=$DST_SOL/linux
DST_CFG=$DST_SOL
DST_IDS=$DST_SOL/usb.ids

# getopt.[ch] are excluded.

FILES_LIB="\
	$TOP/src/usbip_network.[ch] \
	$TOP/libsrc/usbip_common.[ch] \
	$TOP/libsrc/usbip_host_api.c \
	$TOP/libsrc/usbip_config.h \
	$TOP/libsrc/usbip_host_common.h \
	$TOP/libsrc/usbip_host_driver.h \
	$TOP/libsrc/usbip_device_driver.h \
	$TOP/libsrc/names.[ch] \
	$TOP/libsrc/list.h \
	$TOP/libusb/stub/*.[ch] \
	$TOP/libusb/libsrc/dummy_device_driver.c \
	$TOP/libusb/os/usbip_os.h \
	$TOP/libusb/os/usbip_sock.h"
FILES_USBIP="\
	$TOP/src/usbip.[ch] \
	$TOP/src/usbip_bind.c \
	$TOP/src/usbip_connect.c \
	$TOP/src/usbip_disconnect.c \
	$TOP/src/usbip_list.c \
	$TOP/src/usbip_unbind.c"
FILES_USBIPD="\
	$TOP/src/usbipd.[ch] \
	$TOP/src/usbipd_dev.c"
FILES_WIN="\
	$TOP/libusb/os/win32/usbip_os.h \
	$TOP/libusb/os/win32/usbip_sock.h"
FILES_LINUX="\
	$INC/uapi/linux/usbip_api.h"
FILE_CFG=$TOP/libusb/os/win32/config.h
FILE_IDS=$HWDATA/usb.ids

cp $FILES_LIB $DST_LIB
cp $FILES_USBIP $DST_USBIP
cp $FILES_USBIPD $DST_USBIPD
cp $FILES_WIN $DST_WIN
cp $FILES_LINUX $DST_LINUX
cp $FILE_CFG $DST_CFG

sed -e 's/\xb4/\x20/' $FILE_IDS > $DST_IDS
echo "AccentAcute character(s) in $DST_IDS has been modified."
