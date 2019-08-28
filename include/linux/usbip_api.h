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
 * USB/IP API to create command(s) and daemons with arbitrary network protocol.
 */

#ifndef __UAPI_LINUX_USBIP_API_H
#define __UAPI_LINUX_USBIP_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Using int rather than size_t and ssize_t for cross OS portability.
 */
struct usbip_sock {
	int fd;
	void *arg;
	void *ux;
	int (*send)(void *arg, void *buf, int len);
	int (*recv)(void *arg, void *buf, int len, int wait_all);
	void (*shutdown)(void *arg);
};

void usbip_sock_init(struct usbip_sock *sock, int fd, void *arg,
	int (*send)(void *arg, void *buf, int len),
	int (*recv)(void *arg, void *buf, int len, int wait_all),
	void (*shutdown)(void *arg));

struct usbip_connection_operations {
	struct usbip_sock *(*open)(const char *host, const char *port,
				   void *opt);
	void (*close)(struct usbip_sock *sock);
	void *opt;
};

void usbip_conn_init(
	struct usbip_sock *(*open)(const char *host, const char *port,
				   void *opt),
	void (*close)(struct usbip_sock *sock),
	void *opt);

void usbip_break_all_connections(void);
void usbip_break_connection(struct usbip_sock *sock);

void usbip_set_use_debug(int val);
void usbip_set_use_stderr(int val);
void usbip_set_use_syslog(int val);

#ifdef USBIP_WITH_LIBUSB
void usbip_set_debug_flags(unsigned long flags);
#endif

int usbipd_recv_pdu(struct usbip_sock *sock,
		    const char *host, const char *port);
int usbipd_driver_open(void);
void usbipd_driver_close(void);

#ifdef __cplusplus
}
#endif

#endif /* !__UAPI_LINUX_USBIP_API_H */
