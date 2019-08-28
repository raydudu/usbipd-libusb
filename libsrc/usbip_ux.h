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

/*
 * USB/IP URB transmission in userspace.
 */

#ifndef __USBIP_UX_H
#define __USBIP_UX_H

#include <pthread.h>
#include <linux/usbip_ux.h>

struct usbip_ux {
	struct usbip_sock *sock;
	int devfd;
	int started;
	pthread_t tx, rx;
	struct usbip_ux_kaddr kaddr;
};

struct usbip_sock;

int usbip_ux_setup(struct usbip_sock *sock);
void usbip_ux_cleanup(struct usbip_sock *sock);
int usbip_ux_try_transfer(struct usbip_sock *sock);
void usbip_ux_interrupt(struct usbip_sock *sock);
void usbip_ux_interrupt_pgrp(void);
int usbip_ux_installed(void);

#endif /* !__USBIP_UX_H */
