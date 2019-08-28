/*
 * Copyright (C) 2015 Nobuo Iwata
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __USBIP_STUB_H
#define __USBIP_STUB_H

#include "usbip_config.h"

#include <stdlib.h>
#include <signal.h>
#include <time.h>
#ifndef USBIP_OS_NO_PTHREAD_H
#include <pthread.h>
#endif
#include <libusb.h>
#include "usbip_host_driver.h"
#include "stub_common.h"
#include "list.h"

struct stub_interface {
	struct usbip_usb_interface uinf;
	uint8_t detached;
	uint8_t claimed;
};

struct stub_endpoint {
	uint8_t nr;
	uint8_t dir; /* LIBUSB_ENDPOINT_IN || LIBUSB_ENDPOINT_OUT */
	uint8_t type; /* LIBUSB_TRANSFER_TYPE_ */
};

struct stub_device {
	libusb_device *dev;
	libusb_device_handle *dev_handle;
	struct usbip_usb_device udev;
	struct usbip_device ud;
	uint32_t devid;
	int num_eps;
	struct stub_endpoint *eps;

	pthread_t tx, rx;

	/*
	 * stub_priv preserves private data of each urb.
	 * It is allocated as stub_priv_cache and assigned to urb->context.
	 *
	 * stub_priv is always linked to any one of 3 lists;
	 *	priv_init: linked to this until the comletion of a urb.
	 *	priv_tx  : linked to this after the completion of a urb.
	 *	priv_free: linked to this after the sending of the result.
	 *
	 * Any of these list operations should be locked by priv_lock.
	 */
	pthread_mutex_t priv_lock;
	struct list_head priv_init;
	struct list_head priv_tx;
	struct list_head priv_free;

	/* see comments for unlinking in stub_rx.c */
	struct list_head unlink_tx;
	struct list_head unlink_free;

	pthread_mutex_t tx_waitq;
	int should_stop;

	struct stub_interface ifs[];
};

/* private data into urb->priv */
struct stub_priv {
	unsigned long seqnum;
	struct list_head list;
	struct stub_device *sdev;
	struct libusb_transfer *trx;

	uint8_t dir;
	uint8_t unlinking;
};

struct stub_unlink {
	unsigned long seqnum;
	struct list_head list;
	enum libusb_transfer_status status;
};

struct stub_edev_data {
	libusb_device *dev;
	struct stub_device *sdev;
	int num_eps;
	struct stub_endpoint eps[];
};

/* stub_rx.c */
void *stub_rx_loop(void *data);

/* stub_tx.c */
void stub_enqueue_ret_unlink(struct stub_device *sdev, uint32_t seqnum,
			     enum libusb_transfer_status status);
void LIBUSB_CALL stub_complete(struct libusb_transfer *trx);
void *stub_tx_loop(void *data);

/* for libusb */
extern libusb_context *stub_libusb_ctx;
uint8_t stub_get_transfer_type(struct stub_device *sdev, uint8_t ep);
uint8_t stub_endpoint_dir(struct stub_device *sdev, uint8_t ep);
int stub_endpoint_dir_out(struct stub_device *sdev, uint8_t ep);
uint8_t stub_get_transfer_flags(uint32_t in);

/* from stub_main.c */
void stub_device_cleanup_transfers(struct stub_device *sdev);
void stub_device_cleanup_unlinks(struct stub_device *sdev);

#endif /* __USBIP_STUB_H */
