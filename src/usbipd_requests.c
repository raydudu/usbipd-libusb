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

#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>


#include "list.h"
#include "usbip_host_driver.h"
#include "usbip_network.h"
#include <usbip_debug.h>

#include "usbipd_requests.h"


static int recv_request_attach(int sock_fd,
                               const char *host, const char *port)
{
	struct usbip_exported_devices edevs;
	struct usbip_exported_device *edev;
	struct op_import_request req;
	struct usbip_usb_device pdu_udev;
	int found = 0;
	int error = 0;
	int rc;

	(void)host;
	(void)port;

	rc = usbip_refresh_device_list(&edevs);
	if (rc < 0) {
		dbg("could not refresh device list: %d", rc);
		goto err_out;
	}

	memset(&req, 0, sizeof(req));

	rc = usbip_net_recv(sock_fd, &req, sizeof(req));
	if (rc < 0) {
		dbg("usbip_net_recv failed: import request");
		goto err_free_edevs;
	}
	PACK_OP_IMPORT_REQUEST(0, &req);

	edev = usbip_get_device(&edevs, req.busid);
	if (edev) {
		info("found requested device: %s", req.busid);
		found = 1;
	}

	if (found) {
		/* export device needs a TCP/IP socket descriptor */
		rc = usbip_export_device(edev, sock_fd);
		if (rc < 0)
			error = 1;
	} else {
		info("requested device not found: %s", req.busid);
		error = 1;
	}

	rc = usbip_net_send_op_common(sock_fd, OP_REP_IMPORT, (!error ? ST_OK : ST_NA));
	if (rc < 0) {
		dbg("usbip_net_send_op_common failed: %#0x", OP_REP_IMPORT);
		goto err_free_edevs;
	}

	if (error) {
		dbg("import request busid %s: failed: %d", req.busid, error);
		goto err_free_edevs;
	}

	memcpy(&pdu_udev, &edev->udev, sizeof(pdu_udev));
    PACK_OP_IMPORT_REPLY(1, pdu_udev);

	rc = usbip_net_send(sock_fd, &pdu_udev, sizeof(pdu_udev));
	if (rc < 0) {
		dbg("usbip_net_send failed: devinfo");
		goto err_free_edevs;
	}

	dbg("import request busid %s: complete", req.busid);

	rc = usbip_try_transfer(edev, sock_fd);
	if (rc < 0) {
		err("try transfer");
		goto err_free_edevs;
	}

	usbip_free_device_list(&edevs);

	return 0;
err_free_edevs:
	usbip_free_device_list(&edevs);
err_out:
	return -1;
}

static int recv_request_devlist(int sock_fd,
                                const char *host, const char *port)
{
	struct usbip_exported_devices edevs;
	struct usbip_exported_device *edev;
	struct usbip_usb_device pdu_udev;
	struct usbip_usb_interface pdu_uinf;
	struct op_devlist_reply reply;
	struct list_head *j;
	int rc, i;

	rc = usbip_refresh_device_list(&edevs);
	if (rc < 0) {
		dbg("could not refresh device list: %d", rc);
		goto err_out;
	}

	reply.ndev = edevs.ndevs;
	info("importable devices: %d", reply.ndev);

	rc = usbip_net_send_op_common(sock_fd, OP_REP_DEVLIST, ST_OK);
	if (rc < 0) {
		dbg("usbip_net_send_op_common failed: %#0x", OP_REP_DEVLIST);
		goto err_free_edevs;
	}

	reply.ndev = htonl(reply.ndev);

	rc = usbip_net_send(sock_fd, &reply, sizeof(reply));
	if (rc < 0) {
		dbg("usbip_net_send failed: %#0x", OP_REP_DEVLIST);
		goto err_free_edevs;
	}

	list_for_each(j, &edevs.edev_list) {
		edev = list_entry(j, struct usbip_exported_device, node);
		dump_usb_device(&edev->udev);
		memcpy(&pdu_udev, &edev->udev, sizeof(pdu_udev));
        PACK_OP_IMPORT_REPLY(1, pdu_udev);

		rc = usbip_net_send(sock_fd, &pdu_udev, sizeof(pdu_udev));
		if (rc < 0) {
			dbg("usbip_net_send failed: pdu_udev");
			goto err_free_edevs;
		}

		for (i = 0; i < edev->udev.bNumInterfaces; i++) {
			dump_usb_interface(&edev->uinf[i]);
			memcpy(&pdu_uinf, &edev->uinf[i], sizeof(pdu_uinf));

			rc = usbip_net_send(sock_fd, &pdu_uinf,
                                sizeof(pdu_uinf));
			if (rc < 0) {
				err("usbip_net_send failed: pdu_uinf");
				goto err_free_edevs;
			}
		}
	}

	usbip_free_device_list(&edevs);

	return 0;
err_free_edevs:
	usbip_free_device_list(&edevs);
err_out:
	return -1;
}

struct usbipd_recv_pdu_op usbipd_recv_pdu_ops[] = {
	{OP_REQ_DEVLIST, recv_request_devlist},
	{OP_REQ_IMPORT,  recv_request_attach},
	{OP_UNSPEC, NULL}
};
