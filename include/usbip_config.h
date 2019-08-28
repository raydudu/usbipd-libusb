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

#ifdef HAVE_CONFIG_H
#include "config.h"

#if !HAVE_DAEMON
#define USBIP_OS_NO_DAEMON
#endif

#if HAVE_DECL_CONTAINER_OF
#define USBIP_OS_HAVE_CONTAINER_OF
#endif

#if !HAVE_DECL_P_TMPDIR
#define USBIP_OS_NO_P_TMPDIR
#endif

#if !HAVE_DIRENT_H
#define USBIP_OS_NO_DIRENT_H
#endif

#if !HAVE_FORK
#define USBIP_OS_NO_FORK
#endif

#if !HAVE_LIBUDEV_H
#define USBIP_OS_NO_LIBUDEV
#endif

#if !HAVE_PTHREAD_H
#define USBIP_OS_NO_PTHREAD_H
#endif

#if !HAVE_POLL_H
#define USBIP_OS_NO_POLL_H
#endif

#if !HAVE_SYS_SOCKET_H
#define USBIP_OS_NO_SYS_SOCKET
#endif

#if !HAVE_SYSLOG
#define USBIP_OS_NO_SYSLOG
#endif

#if !HAVE_UNISTD_H
#define USBIP_OS_NO_UNISTD_H
#endif

#endif /* HAVE_CONFIG_H */

#endif /* !__USBIP_CONFIG_H */
