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

#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <string.h>
#include <usbip_debug.h>
#include <errno.h>

#include "usbip_config.h"

#include "usbip_network.h"

int usbip_port = 3240;
char *usbip_port_string = "3240";

void usbip_setup_port_number(char *arg)
{
	char *end;
	int port = (int)strtol(arg, &end, 10);

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

/* Receive data over TCP/IP. *//*

int usbip_recv(struct usbip_device *ud, void *buf, int size) {
	int result;
	int total = 0;

	*/
/* for blocks of if (usbip_dbg_flag_xmit) *//*

	char *bp = (char *)buf;
	int osize = size;

	if (!ud->sock_fd || !buf || !size) {
		err("invalid arg, sock %d buff %p size %d",
               ud->sock_fd, buf, size);
		errno = EINVAL;
		return -1;
	}

	do {
		usbip_dbg_xmit("receiving %d", size);
        result = recv(ud->sock_fd, bp, size, 0);

		if (result < 0) {
		    if (errno == EAGAIN ||  errno == EINTR) {
		        result = 0; // Try more
		    } else {
                err("receive error %d (errno %d)", result, errno);
                goto out;
            }
		} else if (result == 0) {
		    info("connection closed, releasing the device...");
		    goto out;
		}

		size -= result;
		bp += result;
		total += result;
	} while (size > 0);

	if (usbip_dbg_flag_xmit) {
		dbg("received, osize %d ret %d size %d total %d",
			 osize, result, size, total);
		usbip_dump_buffer((char *)buf, osize);
	}

	return total;

out:
	return result;
}
*/
ssize_t usbip_net_recv(int sock_fd, void *buff, size_t bufflen) {
    size_t recvd  = 0;

    while(recvd != bufflen) {
        int ret = recv(sock_fd, (char *)buff + recvd, bufflen - recvd, MSG_WAITALL);
        if (ret < 0) {
            if (errno == EAGAIN ||  errno == EINTR) {
                ret = 0; // Try more
            } else {
                err("receive error %d (errno %d)", ret, errno);
                break;
            }
            return ret;
        } else if (ret == 0) {
            info("connection closed, releasing the device...");
            return 0;
        }

        recvd += ret;
    }

    return recvd;
}

ssize_t usbip_net_send(int sock_fd, void *buff, size_t bufflen) {
    ssize_t total = 0;

    while (bufflen > 0) {
        ssize_t ret = send(sock_fd, buff, bufflen, 0);

        if (ret <= 0) {
            return -1;
        }

        buff	 = (void *)((intptr_t) buff + ret);
        bufflen	-= ret;
        total	+= ret;
    }

    return total;
}

/* Send data over TCP/IP. */
int usbip_net_sendvec(int fd, struct iovec *vec, size_t num) {
    if (usbip_dbg_flag_xmit) {
        size_t i;
        for (i = 0; i < num; i++) {
            dbg("sending, idx %zd size %zd", i, vec[i].iov_len);
            usbip_dump_buffer(vec[i].iov_base, vec[i].iov_len);
        }
    }
    return writev(fd, vec, num);
}
int usbip_net_send_op_common(int sock_fd,
			     uint32_t code, uint32_t status)
{
	struct op_common op_common;
	int rc;

	memset(&op_common, 0, sizeof(op_common));

	op_common.version = htons(USBIP_VERSION);
	op_common.code    = htons(code);
	op_common.status  = htonl(status);

	rc = usbip_net_send(sock_fd, &op_common, sizeof(op_common));
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

int usbip_net_recv_op_common(int sock_fd, uint16_t *code)
{
	struct op_common op_common;
	int rc;

	memset(&op_common, 0, sizeof(op_common));

	rc = usbip_net_recv(sock_fd, &op_common, sizeof(op_common));
	if (rc < 0) {
		dbg("usbip_net_recv failed: %d", rc);
		goto err;
	}

    op_common.version = ntohs(op_common.version);
    op_common.code    = ntohs(op_common.code);
    op_common.status  = ntohl(op_common.status);

	if (op_common.version != USBIP_VERSION) {
		dbg("version mismatch: %d %d", op_common.version, USBIP_VERSION);
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

	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
	if (ret < 0)
		dbg("setsockopt: SO_REUSEADDR");

	return ret;
}

int usbip_net_set_nodelay(int sockfd)
{
	const int val = 1;
	int ret;

	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
	if (ret < 0)
		dbg("setsockopt: TCP_NODELAY");

	return ret;
}

int usbip_net_set_keepalive(int sockfd)
{
	const int val = 1;
	int ret;

	ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
	if (ret < 0)
		dbg("setsockopt: SO_KEEPALIVE");

	return ret;
}

int usbip_net_set_v6only(int sockfd)
{
	const int val = 1;
	int ret;

	ret = setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &val,
			   sizeof(val));
	if (ret < 0)
		dbg("setsockopt: IPV6_V6ONLY");

	return ret;
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
