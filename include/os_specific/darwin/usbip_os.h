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

#ifndef _USBIP_DARWIN_OS_H
#define _USBIP_DARWIN_OS_H

#include <signal.h>
#include <poll.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#define USBIP_OS_NO_DAEMON /* deplicated */

#define SIGCLD SIGCHLD

static inline int ppoll(struct pollfd *fds, nfds_t nfds,
			const struct timespec *timeout_ts,
			const sigset_t *sigmask)
{
	sigset_t origmask;
	int ready, timeout;

	timeout = (timeout_ts == NULL) ? -1 :
		(timeout_ts->tv_sec * 1000 + timeout_ts->tv_nsec / 1000000);
	sigprocmask(SIG_SETMASK, sigmask, &origmask);
	ready = poll(fds, nfds, timeout);
	sigprocmask(SIG_SETMASK, &origmask, NULL);
	return ready;
}

/*
 * Copied from /usr/include/linux/usb/ch9.h
 */
enum usb_device_speed {
	USB_SPEED_UNKNOWN = 0,                  /* enumerating */
	USB_SPEED_LOW, USB_SPEED_FULL,          /* usb 1.1 */
	USB_SPEED_HIGH,                         /* usb 2.0 */
	USB_SPEED_WIRELESS,                     /* wireless (usb 2.5) */
	USB_SPEED_SUPER,                        /* usb 3.0 */
};

/*
 * Copied from /usr/include/linux/usbip.h
 */
enum usbip_device_status {
	/* sdev is available. */
	SDEV_ST_AVAILABLE = 0x01,
	/* sdev is now used. */
	SDEV_ST_USED,
	/* sdev is unusable because of a fatal error. */
	SDEV_ST_ERROR,

	/* vdev does not connect a remote device. */
	VDEV_ST_NULL,
	/* vdev is used, but the USB address is not assigned yet */
	VDEV_ST_NOTASSIGNED,
	VDEV_ST_USED,
	VDEV_ST_ERROR
};

#endif /* !_USBIP_DARWIN_OS_H */
