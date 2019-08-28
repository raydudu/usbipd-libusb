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

#ifndef _USBIP_WIN32_OS_H
#define _USBIP_WIN32_OS_H

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <tchar.h>
#include <windows.h>
#include <process.h>
#include <signal.h>

#define UNUSED
#define PACK(__Declaration__) \
__pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))

#define inline __inline

#undef __reserved

#define u_int8_t uint8_t
#define u_int16_t uint16_t
#define ssize_t int
#define snprintf _snprintf
#define container_of(ptr, type, member) \
	((type *)((unsigned char *)ptr - (unsigned long)&((type *)0)->member))


#define dbg_fmt(fmt)    pr_fmt("%s:%d:[%s] " fmt), "debug",     \
			__FILE__, __LINE__, __FUNCTION__

#define err(fmt, ...) \
	do { \
		if (usbip_use_stderr) { \
			fprintf(stderr, pr_fmt(fmt), "error", __VA_ARGS__); \
		} \
	} while (0)
#define info(fmt, ...) \
	do { \
		if (usbip_use_stderr) { \
			fprintf(stderr, pr_fmt(fmt), "info", __VA_ARGS__); \
		} \
	} while (0)
#define dbg(fmt, ...) \
	do { \
	if (usbip_use_debug) { \
		if (usbip_use_stderr) { \
			fprintf(stderr, dbg_fmt(fmt), __VA_ARGS__); \
		} \
	} \
	} while (0)

#define pr_debug(...) \
	fprintf(stdout, __VA_ARGS__)
#define pr_err(...) \
	fprintf(stderr, __VA_ARGS__)
#define pr_warn(...) \
	fprintf(stderr, __VA_ARGS__)

#define dev_dbg(dev, ...) \
	(usbip_dev_printf(stdout, "DEBUG", dev), \
	fprintf(stdout, __VA_ARGS__))
#define dev_info(dev, ...) \
	(usbip_dev_printf(stdout, "INFO", dev), \
	fprintf(stdout, __VA_ARGS__))
#define dev_err(dev, ...) \
	(usbip_dev_printf(stderr, "ERROR", dev), \
	fprintf(stderr, __VA_ARGS__))

#define devh_dbg(devh, ...) \
	(usbip_devh_printf(stdout, "DEBUG", devh), \
	fprintf(stdout, __VA_ARGS__))
#define devh_info(devh, ...) \
	(usbip_devh_printf(stdout, "INFO", devh), \
	fprintf(stdout, __VA_ARGS__))
#define devh_err(devh, ...) \
	(usbip_devh_printf(stderr, "ERROR", devh), \
	fprintf(stderr, __VA_ARGS__))

enum {
	usbip_debug_xmit	= (1 << 0),
	usbip_debug_sysfs	= (1 << 1),
	usbip_debug_urb		= (1 << 2),
	usbip_debug_eh		= (1 << 3),

	usbip_debug_stub_cmp	= (1 << 8),
	usbip_debug_stub_dev	= (1 << 9),
	usbip_debug_stub_rx	= (1 << 10),
	usbip_debug_stub_tx	= (1 << 11),
};

#define usbip_dbg_flag_xmit	(usbip_debug_flag & usbip_debug_xmit)
#define usbip_dbg_flag_stub_rx	(usbip_debug_flag & usbip_debug_stub_rx)
#define usbip_dbg_flag_stub_tx	(usbip_debug_flag & usbip_debug_stub_tx)

#define usbip_dbg_with_flag(flag, fmt, ...)		\
	do {						\
		if (flag & usbip_debug_flag)		\
			fprintf(stdout, fmt, __VA_ARGS__);	\
	} while (0)

#define usbip_dbg_sysfs(fmt, ...) \
	usbip_dbg_with_flag(usbip_debug_sysfs, fmt, __VA_ARGS__)
#define usbip_dbg_xmit(fmt, ...) \
	usbip_dbg_with_flag(usbip_debug_xmit, fmt, __VA_ARGS__)
#define usbip_dbg_urb(fmt, ...) \
	usbip_dbg_with_flag(usbip_debug_urb, fmt, __VA_ARGS__)
#define usbip_dbg_eh(fmt, ...) \
	usbip_dbg_with_flag(usbip_debug_eh, fmt, __VA_ARGS__)

#define usbip_dbg_stub_cmp(fmt, ...) \
	usbip_dbg_with_flag(usbip_debug_stub_cmp, fmt, __VA_ARGS__)
#define usbip_dbg_stub_rx(fmt, ...) \
	usbip_dbg_with_flag(usbip_debug_stub_rx, fmt, __VA_ARGS__)
#define usbip_dbg_stub_tx(fmt, ...) \
	usbip_dbg_with_flag(usbip_debug_stub_tx, fmt, __VA_ARGS__)

#undef ENOENT
#undef ECONNRESET
#undef ETIMEDOUT
#undef EPIPE
#undef EOVERFLOW
#define ENOENT 2
#define ECONNRESET 104
#define ETIMEDOUT 110
#define EPIPE 32
#define ESHUTDOWN 108
#define EOVERFLOW 75

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

#define socket_start() \
	do { \
		WSADATA wsa; \
		int ret = WSAStartup(MAKEWORD(2, 0), &wsa); \
		if (ret) { \
			dbg("failed to start winsock"); \
		} \
	} while (0)
#define socket_stop() \
	WSACleanup()
#define socket_close(sd) \
	closesocket(sd)
#define __send(sd, buf, len, flags) \
	send((sd), (char *)(buf), (len), (flags))
#define __recv(sd, buf, len, flags) \
	recv((sd), (char *)(buf), (len), (flags))
#define __setsockopt(sd, level, opt, val, len) \
	setsockopt((sd), (level), (opt), (const char *)(val), (len))

#define EAI_SYSTEM -1

struct timespec {
	ULONG tv_sec;
	ULONG tv_nsec;
};

#define ppoll(fds, nfds, tout, sigmask) \
	WSAPoll((fds), (nfds), (((tout)->tv_sec) * 1000))

struct sigaction {
	void (*sa_handler)(int);
};

#define SIGCLD 0

#define sigaction(signum, act, oldact) \
	do { \
		if (signum) { \
			signal((signum), (act)->sa_handler); \
		} \
	} while (0)

#define sigset_t int

#define sigemptyset(set) do {} while (0)
#define sigfillset(set) do {} while (0)
#define sigdelset(set, num) (*(set) = (num))

const char *strsignal(int num);

struct pthread_mutex {
	HANDLE handle;
};

#define pthread_mutex_t struct pthread_mutex
#define pthread_mutexattr_t void

static inline
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	mutex->handle = CreateSemaphore(NULL, 1, 1, NULL);
	if (mutex->handle == NULL)
		return -1;
	return 0;
}

static inline
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	if (!CloseHandle(mutex->handle))
		return -1;
	return 0;
}

static inline
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	if (WaitForSingleObject(mutex->handle,  INFINITE) == WAIT_FAILED)
		return -1;
	return 0;
}

static inline
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	if (!ReleaseSemaphore(mutex->handle, 1, NULL))
		return -1;
	return 0;
}

#define pthread_t HANDLE
#define pthread_attr_t void

static inline
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
		   void *(*start_routine)(void *), void *arg)
{
	*thread = CreateThread(NULL, 0,
			       (LPTHREAD_START_ROUTINE)start_routine,
			       arg, 0, NULL);
	if (*thread == NULL)
		return -1;
	return 0;
}

static inline
int pthread_join(pthread_t thread, void **retval)
{
	WaitForSingleObject(thread, INFINITE);
	CloseHandle(thread);
}

static inline
void pthread_exit(void)
{
	ExitThread(0);
}

static inline
const char *strsignal(int num)
{
	switch (num) {
	case SIGABRT:
		return "SIGABRT";
	case SIGFPE:
		return "SIGFPE";
	case SIGILL:
		return "SIGILL";
	case SIGINT:
		return "SIGINT";
	case SIGSEGV:
		return "SIGSEGV";
	case SIGTERM:
		return "SIGTERM";
	}
	return "unknown";
}

static inline
void get_tmp_dir(char *dir, int len)
{
	int i, bytes;
	TCHAR buf[128];

	GetTempPath((sizeof(buf) / 2) - 1, buf);
	bytes = WideCharToMultiByte(CP_ACP, 0, buf, -1, dir, len, NULL, NULL);
	for (i = 0; i < bytes; i++) {
		if (dir[i] == '\\')
			dir[i] = '/';
	}
}

#endif /* !_USBIP_WIN32_OS_H */
