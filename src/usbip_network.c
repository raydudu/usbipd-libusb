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

#include "usbip_config.h"

#ifndef USBIP_OS_NO_SYS_SOCKET
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#endif

#include <string.h>

#include "usbip_common.h"
#include "usbip_network.h"

int usbip_port = 3240;
char *usbip_port_string = "3240";

void usbip_setup_port_number(char *arg)
{
	char *end;
	unsigned long int port = strtoul(arg, &end, 10);

	dbg("parsing port arg '%s'", arg);
	if (end == arg) {
		err("port: could not parse '%s' as a decimal integer", arg);
		return;
	}

	if (*end != '\0') {
		err("port: garbage at end of '%s'", arg);
		return;
	}

	if (port > UINT16_MAX) {
		err("port: %s too high (max=%d)",
		    arg, UINT16_MAX);
		return;
	}

	usbip_port = port;
	usbip_port_string = arg;
	info("using port %d (\"%s\")", usbip_port, usbip_port_string);
}

void usbip_net_pack_uint32_t(int pack, uint32_t *num)
{
	uint32_t i;

	if (pack)
		i = htonl(*num);
	else
		i = ntohl(*num);

	*num = i;
}

void usbip_net_pack_uint16_t(int pack, uint16_t *num)
{
	uint16_t i;

	if (pack)
		i = htons(*num);
	else
		i = ntohs(*num);

	*num = i;
}

void usbip_net_pack_usb_device(int pack, struct usbip_usb_device *udev)
{
	usbip_net_pack_uint32_t(pack, &udev->busnum);
	usbip_net_pack_uint32_t(pack, &udev->devnum);
	usbip_net_pack_uint32_t(pack, &udev->speed);

	usbip_net_pack_uint16_t(pack, &udev->idVendor);
	usbip_net_pack_uint16_t(pack, &udev->idProduct);
	usbip_net_pack_uint16_t(pack, &udev->bcdDevice);
}

void usbip_net_pack_usb_interface(int pack UNUSED,
				  struct usbip_usb_interface *udev UNUSED)
{
	/* uint8_t members need nothing */
}

static ssize_t usbip_net_xmit(struct usbip_sock *sock, void *buff,
			      size_t bufflen, int sending)
{
	ssize_t nbytes;
	ssize_t total = 0;

	if (!bufflen)
		return 0;

	do {
		if (sending) {
			if (sock->send)
				nbytes = sock->send(sock->arg, buff, bufflen);
			else
				nbytes = __send(sock->fd, buff, bufflen, 0);
		} else {
			if (sock->recv)
				nbytes = sock->recv(sock->arg, buff, bufflen,
						    1);
			else
				nbytes = __recv(sock->fd, buff, bufflen,
						MSG_WAITALL);
		}

		if (nbytes <= 0) {
			if (!sending && nbytes == 0)
				dbg("received zero - broken connection?");
			return -1;
		}

		buff	 = (void *)((intptr_t) buff + nbytes);
		bufflen	-= nbytes;
		total	+= nbytes;

	} while (bufflen > 0);

	return total;
}

ssize_t usbip_net_recv(struct usbip_sock *sock, void *buff, size_t bufflen)
{
	return usbip_net_xmit(sock, buff, bufflen, 0);
}

ssize_t usbip_net_send(struct usbip_sock *sock, void *buff, size_t bufflen)
{
	return usbip_net_xmit(sock, buff, bufflen, 1);
}

int usbip_net_send_op_common(struct usbip_sock *sock,
			     uint32_t code, uint32_t status)
{
	struct op_common op_common;
	int rc;

	memset(&op_common, 0, sizeof(op_common));

	op_common.version = USBIP_VERSION;
	op_common.code    = code;
	op_common.status  = status;

	PACK_OP_COMMON(1, &op_common);

	rc = usbip_net_send(sock, &op_common, sizeof(op_common));
	if (rc < 0) {
		dbg("usbip_net_send failed: %d", rc);
		return -1;
	}

	return 0;
}

struct op_common_error {
	uint32_t	status;
	const char	*str;
};

struct op_common_error op_common_errors[] = {
	{ST_NA, "not available"},
	{ST_NO_FREE_PORT, "no free port"},
	{ST_DEVICE_NOT_FOUND, "device not found"},
	{0, NULL}
};

static const char *op_common_strerror(uint32_t status)
{
	struct op_common_error *err;

	for (err = op_common_errors; err->str != NULL; err++) {
		if (err->status == status)
			return err->str;
	}
	return "unknown error";
}

int usbip_net_recv_op_common(struct usbip_sock *sock, uint16_t *code)
{
	struct op_common op_common;
	int rc;

	memset(&op_common, 0, sizeof(op_common));

	rc = usbip_net_recv(sock, &op_common, sizeof(op_common));
	if (rc < 0) {
		dbg("usbip_net_recv failed: %d", rc);
		goto err;
	}

	PACK_OP_COMMON(0, &op_common);

	if (op_common.version != USBIP_VERSION) {
		dbg("version mismatch: %d %d", op_common.version,
		    USBIP_VERSION);
		goto err;
	}

	switch (*code) {
	case OP_UNSPEC:
		break;
	default:
		if (op_common.code != *code) {
			dbg("unexpected pdu %#0x for %#0x", op_common.code,
			    *code);
			goto err;
		}
	}

	if (op_common.status != ST_OK) {
		dbg("request failed at peer: %s",
			op_common_strerror(op_common.status));
		goto err;
	}

	*code = op_common.code;

	return 0;
err:
	return -1;
}

int usbip_net_set_reuseaddr(int sockfd)
{
	const int val = 1;
	int ret;

	ret = __setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
	if (ret < 0)
		dbg("setsockopt: SO_REUSEADDR");

	return ret;
}

int usbip_net_set_nodelay(int sockfd)
{
	const int val = 1;
	int ret;

	ret = __setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
	if (ret < 0)
		dbg("setsockopt: TCP_NODELAY");

	return ret;
}

int usbip_net_set_keepalive(int sockfd)
{
	const int val = 1;
	int ret;

	ret = __setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
	if (ret < 0)
		dbg("setsockopt: SO_KEEPALIVE");

	return ret;
}

int usbip_net_set_v6only(int sockfd)
{
	const int val = 1;
	int ret;

	ret = __setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &val,
			   sizeof(val));
	if (ret < 0)
		dbg("setsockopt: IPV6_V6ONLY");

	return ret;
}

/*
 * IPv6 Ready
 */
static struct usbip_sock *net_tcp_open(const char *hostname,
				       const char *service,
				       void *opt UNUSED)
{
	struct addrinfo hints, *res, *rp;
	int sockfd;
	struct usbip_sock *sock;
	int ret;

	socket_start();

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* get all possible addresses */
	ret = getaddrinfo(hostname, service, &hints, &res);
	if (ret < 0) {
		dbg("getaddrinfo: %s service %s: %s", hostname, service,
		    usbip_net_gai_strerror(ret));
		goto err_stop;
	}

	/* try the addresses */
	for (rp = res; rp; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);
		if (sockfd < 0)
			continue;

		/* should set TCP_NODELAY for usbip */
		usbip_net_set_nodelay(sockfd);
		/* TODO: write code for heartbeat */
		usbip_net_set_keepalive(sockfd);

		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;

		socket_close(sockfd);
	}

	freeaddrinfo(res);

	if (!rp)
		goto err_stop;

	sock = (struct usbip_sock *)malloc(sizeof(struct usbip_sock));
	if (!sock) {
		dbg("Fail to malloc usbip_sock");
		goto err_close;
	}
	usbip_sock_init(sock, sockfd, NULL, NULL, NULL, NULL);

	return sock;

err_close:
	socket_close(sockfd);
err_stop:
	socket_stop();
	return NULL;
}

static void net_tcp_close(struct usbip_sock *sock)
{
	if (sock->shutdown)
		sock->shutdown(sock->arg);
	else
		socket_close(sock->fd);
	free(sock);
	socket_stop();
}

void usbip_net_tcp_conn_init(void)
{
	usbip_conn_init(net_tcp_open, net_tcp_close, NULL);
}

static const char *gai_unknown_error = "?";

const char *usbip_net_gai_strerror(int errcode)
{
	const char *s;

	s = gai_strerror(errcode);
	if (s == NULL)
		return gai_unknown_error;
	return s;
}
