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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "usbip_common.h"
#include "usbip_ux.h"

#undef  PROGNAME
#define PROGNAME "libusbip"

#define DEVNAME "/dev/" USBIP_UX_DEV_NAME

#define BLEN 1500

#ifdef DEBUG
void dump_buff(char *buff, size_t bufflen, char *label)
{
#define DUMP_BUFF 80
#define WORK_BUFF 16
	size_t i = 0, j;
	char b[DUMP_BUFF];
	char bb[WORK_BUFF];

	dbg("dump %s for %zd bytes", label, bufflen);
	for (i = 0; i < bufflen; i++) {
		j = i % 16;
		if (j == 0) {
			b[0] = 0;
			sprintf(bb, "%04zx  %02x", i, *(buff+i) & 0xff);
		} else if (j == 8)
			sprintf(bb, "  %02x", *(buff+i) & 0xff);
		else
			sprintf(bb, " %02x", *(buff+i) & 0xff);

		strncat(b, bb, WORK_BUFF);
		if (j == 15 || i == bufflen-1)
			dbg("%s", b);
	}
}
#else
#define dump_buff(buff, bufflen, label) do {} while (0)
#endif

static void *usbip_ux_rx(void *arg)
{
	struct usbip_ux *ux = (struct usbip_ux *)arg;
	ssize_t received, written, ret;
	int good = 1;
	char buf[BLEN];

	while (good) {
		if (ux->sock->recv)
			received = ux->sock->recv(ux->sock->arg, buf, BLEN, 0);
		else
			received = recv(ux->sock->fd, buf, BLEN, 0);

		if (received == 0) {
			dbg("connection closed on sock:%s", ux->kaddr.sock);
			break;
		} else if (received < 0) {
			dbg("receive error on sock:%s", ux->kaddr.sock);
			break;
		}
		dump_buff(buf, received, "ux received");
		written = 0;
		while (written < received) {
			ret = write(ux->devfd, buf+written, received-written);
			if (ret < 0) {
				dbg("write error for sock:%s", ux->kaddr.sock);
				good = 0;
				break;
			}
			written += ret;
		}
	}
	dbg("end of ux-rx for sock:%s", ux->kaddr.sock);
	ioctl(ux->devfd, USBIP_UX_IOCINTR);
	return 0;
}

static void *usbip_ux_tx(void *arg)
{
	struct usbip_ux *ux = (struct usbip_ux *)arg;
	ssize_t sent, reads;
	char buf[BLEN];

	for (;;) {
		reads = read(ux->devfd, buf, BLEN);
		if (reads == 0) {
#ifdef DEBUG
			dbg("end of read on sock:%s continue.", ux->kaddr.sock);
#endif
			sched_yield();
			continue;
		} else if (reads < 0) {
			dbg("read error on sock:%s", ux->kaddr.sock);
			break;
		}
		dump_buff(buf, reads, "ux sending");
		if (ux->sock->send)
			sent = ux->sock->send(ux->sock->arg, buf, reads);
		else
			sent = send(ux->sock->fd, buf, reads, 0);

		if (sent < 0) {
			dbg("connection closed on sock:%s", ux->kaddr.sock);
			break;
		} else if (sent < reads) {
			dbg("send error on sock:%s %zd < %zd", ux->kaddr.sock,
				sent, reads);
			break;
		}
	}
	dbg("end of ux-tx for sock:%s", ux->kaddr.sock);
	if (ux->sock->shutdown)
		ux->sock->shutdown(ux->sock->arg);
	else
		shutdown(ux->sock->fd, SHUT_RDWR);
	return 0;
}

/*
 * Setup user space mode.
 * Null will be set in ux if usbip_ux.ko is not installed.
 */
int usbip_ux_setup(struct usbip_sock *sock)
{
	struct usbip_ux *ux;
	int fd, ret;

	if (sock->ux)
		return 0;

	fd = open(DEVNAME, O_RDWR);
	if (fd < 0) {
		dbg("failed to open %s", DEVNAME);
		dbg("URBs will be transferred in kernel space");
		return 0;
	}
	ux = (struct usbip_ux *)malloc(sizeof(struct usbip_ux));
	if (ux == NULL) {
		dbg("failed to alloc ux data");
		goto err_close;
	}
	sock->ux = ux;
	ux->devfd = fd;
	ux->started = 0;
	ux->sock = sock;
	ret = ioctl(ux->devfd, USBIP_UX_IOCSETSOCKFD, sock->fd);
	if (ret) {
		dbg("failed to set sock fd");
		goto err_free;
	}
	ret = ioctl(ux->devfd, USBIP_UX_IOCGETKADDR, &ux->kaddr);
	if (ret) {
		dbg("failed to get kaddr");
		goto err_free;
	}
	dbg("successfully prepared userspace transmission sock:%s ux:%s pid:%d",
		 ux->kaddr.sock, ux->kaddr.ux, getpid());

	return 0;
err_free:
	free(ux);
err_close:
	close(ux->devfd);
	return -1;
}

/*
 * Only for error handling before start.
 */
void usbip_ux_cleanup(struct usbip_sock *sock)
{
	struct usbip_ux *ux = sock->ux;

	if (ux == NULL)
		return;

	close((ux)->devfd);
	free(ux);
	sock->ux = NULL;
}

/*
 * Starts transmission threads.
 */
static int usbip_ux_start(struct usbip_sock *sock)
{
	struct usbip_ux *ux = sock->ux;
	int ret;

	if (ux == NULL)
		return 0;

	ret = pthread_create(&ux->rx, NULL, usbip_ux_rx, ux);
	if (ret) {
		dbg("failed to start recv thread");
		goto err;
	}
	ret = pthread_create(&ux->tx, NULL, usbip_ux_tx, ux);
	if (ret) {
		dbg("failed to start send thread");
		goto err;
	}
	ux->started = 1;
	dbg("successfully started userspace transmission");
	return 0;
err:
	close(ux->devfd);
	return -1;
}

/*
 * Waits end of usespace transmission.
 * Will return on following conditions.
 * 1) Detached or unbound
 * 2) Broken connection
 * 3) Closed usbip-ux device
 */
static void usbip_ux_join(struct usbip_sock *sock)
{
	struct usbip_ux *ux = sock->ux;

	if (ux == NULL)
		return;

	dbg("waiting on userspace transmission threads");
	pthread_join((ux)->tx, NULL);
	pthread_join((ux)->rx, NULL);
}

int usbip_ux_try_transfer(struct usbip_sock *sock)
{
	if (usbip_ux_setup(sock))
		return -1;
	usbip_ux_start(sock);
	usbip_ux_join(sock);
	usbip_ux_cleanup(sock);
	return 0;
}

void usbip_ux_interrupt(struct usbip_sock *sock)
{
	struct usbip_ux *ux = sock->ux;

	if (ux == NULL)
		return;
	ioctl((ux)->devfd, USBIP_UX_IOCINTR);
}

void usbip_ux_interrupt_pgrp(void)
{
	int fd;

	fd = open(DEVNAME, O_RDWR);
	if (fd < 0) {
		dbg("failed to open %s", DEVNAME);
		return;
	}
	ioctl(fd, USBIP_UX_IOCINTRPGRP);
	close(fd);
}

int usbip_ux_installed(void)
{
	struct stat buf;

	if (stat(DEVNAME, &buf))
		return 0;
	return 1;
}
