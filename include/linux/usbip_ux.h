/*
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __UAPI_LINUX_USBIP_UX_H
#define __UAPI_LINUX_USBIP_UX_H

#define USBIP_UX_MINOR		0
#define USBIP_UX_CLASS_NAME	"usbip"
#define USBIP_UX_DEV_NAME	"usbip-ux"
#define USBIP_UX_KADDR_LEN	16

struct usbip_ux_kaddr {
	char ux[USBIP_UX_KADDR_LEN+1];
	char sock[USBIP_UX_KADDR_LEN+1];
};

#define USBIP_UX_MAGIC_IOC 'm'

#define USBIP_UX_IOCSETSOCKFD \
		_IOW(USBIP_UX_MAGIC_IOC, 1, int)
#define USBIP_UX_IOCINTR \
		_IO(USBIP_UX_MAGIC_IOC, 2)
#define USBIP_UX_IOCINTRPGRP \
		_IO(USBIP_UX_MAGIC_IOC, 3)
#define USBIP_UX_IOCGETKADDR \
		_IOR(USBIP_UX_MAGIC_IOC, 4, struct usbip_ux_kaddr)

#endif /* __UAPI_LINUX_USBIP_UX_H */
