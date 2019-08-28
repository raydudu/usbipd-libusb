/*
 * Copyright (C) 2011 matt mooney <mfm@muteddisk.com>
 *               2005-2007 Takahiro Hirofuchi
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

#ifndef __USBIPD_H
#define __USBIPD_H

#include "usbip_common.h"

extern unsigned long usbip_libusb_debug;

extern char *usbip_progname;
extern char *usbip_default_pid_file;

struct usbipd_recv_pdu_op {
	uint16_t code;
	int (*proc)(int sock_fd,
		    const char *host, const char *port);
};

extern struct usbipd_recv_pdu_op usbipd_recv_pdu_ops[];

#endif /* __USBIPD_H */
