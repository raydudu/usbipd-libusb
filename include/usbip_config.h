/*
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __USBIP_CONFIG_H
#define __USBIP_CONFIG_H

/* binary-coded decimal version number */
#define USBIP_VERSION 0x00000111

/* Version number of package */
#define VERSION "2.0"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "usbip-libusb 2.0"
#define PACKAGE "usbip-libusb"

#define USBIDS_FILE "/usr/share/hwdata/usb.ids"

#define USBIP_OS_NO_DAEMON
//#define USBIP_OS_HAVE_CONTAINER_OF // list.h config
#define USBIP_OS_NO_FORK
//#define USBIP_OS_NO_PTHREAD_H //TODO Just removes includes, doesn't switch anything else, might be useful in future
//#define USBIP_OS_NO_POLL_H //TODO Just removes includes, doesn't switch anything else, might be useful in future
//#define USBIP_OS_NO_SYS_SOCKET
#define CONFIG_USBIP_DEBUG //Enable all liubusb debug messages


#endif /* !__USBIP_CONFIG_H */
