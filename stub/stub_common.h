/*
 * Copyright (C) 2003-2008 Takahiro Hirofuchi
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
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

#ifndef __STUB_COMMON_H
#define __STUB_COMMON_H

#include "usbip_config.h"

#include <stdio.h>
#include <stddef.h>
#include <libusb-1.0/libusb.h>

#include "usbip_common.h"

struct kvec {
	void *iov_base; /* and that should *never* hold a userland pointer */
	size_t iov_len;
};

/* alternate of kthread_should_stop */
#define stub_should_stop(sdev) ((sdev)->should_stop)

/*
 * See also from include/uapi/linux/usb/ch9.h
 */
#define USB_ENDPOINT_HALT	0 /* IN/OUT will STALL */

#define USB_ENDPOINT_NUMBER_MASK	0x0f /* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK		0x80

#define USB_DIR_OUT             0	/* to device */
#define USB_DIR_IN              0x80	/* to host */

#ifndef USB_TYPE_MASK
#define USB_TYPE_MASK		0x60
#endif

#define USB_RECIP_MASK		0x1f

#define URB_SHORT_NOT_OK	0x0001  /* report short reads as errors */
#define URB_ISO_ASAP		0x0002  /* iso-only; use the first unexpired */
					/* slot in the schedule */
#define URB_NO_TRANSFER_DMA_MAP	0x0004  /* urb->transfer_dma valid on submit */
#define URB_NO_FSBR		0x0020  /* UHCI-specific */
#define URB_ZERO_PACKET		0x0040  /* Finish bulk OUT with short packet */
#define URB_NO_INTERRUPT	0x0080  /* HINT: no non-error interrupt */
					/* needed */
#define URB_FREE_BUFFER		0x0100

#define USB_CLASS_HUB		9

static inline uint8_t get_request_dir(uint8_t rt)
{
	return (rt & USB_ENDPOINT_DIR_MASK);
}

static inline uint8_t get_request_type(uint8_t rt)
{
	return (rt & USB_TYPE_MASK);
}

static inline uint8_t get_recipient(uint8_t rt)
{
	return (rt & USB_RECIP_MASK);
}

#define USB_ENDPOINT_XFERTYPE_MASK	0x03    /* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL	0
#define USB_ENDPOINT_XFER_ISOC		1
#define USB_ENDPOINT_XFER_BULK		2
#define USB_ENDPOINT_XFER_INT		3
#define USB_ENDPOINT_MAX_ADJUSTABLE	0x80

/*
 * See also from include/uapi/linux/usb/ch11.h
 */

#define USB_RT_HUB \
	(LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE)
#define USB_RT_PORT \
	(LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_OTHER)

/*
 * Port feature numbers
 * See USB 2.0 spec Table 11-17
 */
#define USB_PORT_FEAT_CONNECTION	0
#define USB_PORT_FEAT_ENABLE		1
#define USB_PORT_FEAT_SUSPEND		2	/* L2 suspend */
#define USB_PORT_FEAT_OVER_CURRENT	3
#define USB_PORT_FEAT_RESET		4
#define USB_PORT_FEAT_L1		5	/* L1 suspend */
#define USB_PORT_FEAT_POWER		8
#define USB_PORT_FEAT_LOWSPEED		9	/* Should never be used */
#define USB_PORT_FEAT_C_CONNECTION	16
#define USB_PORT_FEAT_C_ENABLE		17
#define USB_PORT_FEAT_C_SUSPEND		18
#define USB_PORT_FEAT_C_OVER_CURRENT	19
#define USB_PORT_FEAT_C_RESET		20
#define USB_PORT_FEAT_TEST		21
#define USB_PORT_FEAT_INDICATOR		22
#define USB_PORT_FEAT_C_PORT_L1		23

int usbip_dev_printf(FILE *s, const char *level,
			struct libusb_device *dev);
int usbip_devh_printf(FILE *s, const char *level,
			libusb_device_handle *dev_handle);

extern unsigned long usbip_debug_flag;
extern struct device_attribute dev_attr_usbip_debug;

#ifndef USBIP_OS_NO_NR_ARGS
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
#define usbip_dbg_flag_vhci_sysfs  (usbip_debug_flag & usbip_debug_vhci_sysfs)


#define usbip_dbg_with_flag(flag, fmt, args...)		\
	do {						\
		if (flag & usbip_debug_flag)		\
			pr_debug(fmt, ##args);		\
	} while (0)

#define usbip_dbg_sysfs(fmt, args...) \
	usbip_dbg_with_flag(usbip_debug_sysfs, fmt, ##args)
#define usbip_dbg_xmit(fmt, args...) \
	usbip_dbg_with_flag(usbip_debug_xmit, fmt, ##args)
#define usbip_dbg_urb(fmt, args...) \
	usbip_dbg_with_flag(usbip_debug_urb, fmt, ##args)
#define usbip_dbg_eh(fmt, args...) \
	usbip_dbg_with_flag(usbip_debug_eh, fmt, ##args)

#define usbip_dbg_stub_cmp(fmt, args...) \
	usbip_dbg_with_flag(usbip_debug_stub_cmp, fmt, ##args)
#define usbip_dbg_stub_rx(fmt, args...) \
	usbip_dbg_with_flag(usbip_debug_stub_rx, fmt, ##args)
#define usbip_dbg_stub_tx(fmt, args...) \
	usbip_dbg_with_flag(usbip_debug_stub_tx, fmt, ##args)
#endif /*! USBIP_OS_NO_NR_ARGS */

/*
 * USB/IP request headers
 *
 * Each request is transferred across the network to its counterpart, which
 * facilitates the normal USB communication. The values contained in the headers
 * are basically the same as in a URB. Currently, four request types are
 * defined:
 *
 *  - USBIP_CMD_SUBMIT: a USB request block, corresponds to usb_submit_urb()
 *    (client to server)
 *
 *  - USBIP_RET_SUBMIT: the result of USBIP_CMD_SUBMIT
 *    (server to client)
 *
 *  - USBIP_CMD_UNLINK: an unlink request of a pending USBIP_CMD_SUBMIT,
 *    corresponds to usb_unlink_urb()
 *    (client to server)
 *
 *  - USBIP_RET_UNLINK: the result of USBIP_CMD_UNLINK
 *    (server to client)
 *
 */
#define USBIP_NOP		0x0000
#define USBIP_CMD_SUBMIT	0x0001
#define USBIP_CMD_UNLINK	0x0002
#define USBIP_RET_SUBMIT	0x0003
#define USBIP_RET_UNLINK	0x0004

#define USBIP_DIR_OUT	0x00
#define USBIP_DIR_IN	0x01

/**
 * struct usbip_header_basic - data pertinent to every request
 * @command: the usbip request type
 * @seqnum: sequential number that identifies requests; incremented per
 *	    connection
 * @devid: specifies a remote USB device uniquely instead of busnum and devnum;
 *	   in the stub driver, this value is ((busnum << 16) | devnum)
 * @direction: direction of the transfer
 * @ep: endpoint number
 */
#pragma pack(4)
struct usbip_header_basic {
	uint32_t command;
	uint32_t seqnum;
	uint32_t devid;
	uint32_t direction;
	uint32_t ep;
};

/**
 * struct usbip_header_cmd_submit - USBIP_CMD_SUBMIT packet header
 * @transfer_flags: URB flags
 * @transfer_buffer_length: the data size for (in) or (out) transfer
 * @start_frame: initial frame for isochronous or interrupt transfers
 * @number_of_packets: number of isochronous packets
 * @interval: maximum time for the request on the server-side host controller
 * @setup: setup data for a control request
 */
#pragma pack(4)
struct usbip_header_cmd_submit {
	uint32_t transfer_flags;
	int32_t transfer_buffer_length;

	/* it is difficult for usbip to sync frames (reserved only?) */
	int32_t start_frame;
	int32_t number_of_packets;
	int32_t interval;

	unsigned char setup[8];
};

/**
 * struct usbip_header_ret_submit - USBIP_RET_SUBMIT packet header
 * @status: return status of a non-iso request
 * @actual_length: number of bytes transferred
 * @start_frame: initial frame for isochronous or interrupt transfers
 * @number_of_packets: number of isochronous packets
 * @error_count: number of errors for isochronous transfers
 */
#pragma pack(4)
struct usbip_header_ret_submit {
	int32_t status;
	int32_t actual_length;
	int32_t start_frame;
	int32_t number_of_packets;
	int32_t error_count;
};

/**
 * struct usbip_header_cmd_unlink - USBIP_CMD_UNLINK packet header
 * @seqnum: the URB seqnum to unlink
 */
#pragma pack(4)
struct usbip_header_cmd_unlink {
	uint32_t seqnum;
};

/**
 * struct usbip_header_ret_unlink - USBIP_RET_UNLINK packet header
 * @status: return status of the request
 */
#pragma pack(4)
struct usbip_header_ret_unlink {
	uint32_t status;
};

/**
 * struct usbip_header - common header for all usbip packets
 * @base: the basic header
 * @u: packet type dependent header
 */
#pragma pack(4)
struct usbip_header {
	struct usbip_header_basic base;

	union {
		struct usbip_header_cmd_submit	cmd_submit;
		struct usbip_header_ret_submit	ret_submit;
		struct usbip_header_cmd_unlink	cmd_unlink;
		struct usbip_header_ret_unlink	ret_unlink;
	} u;
};

/*
 * This is the same as usb_iso_packet_descriptor but packed for pdu.
 */
#pragma pack(4)
struct usbip_iso_packet_descriptor {
	uint32_t offset;
	uint32_t length;			/* expected length */
	uint32_t actual_length;
	uint32_t status;
};

enum usbip_side {
	USBIP_VHCI,
	USBIP_STUB,
};

/* event handler */
#define USBIP_EH_SHUTDOWN	(1 << 0)
#define USBIP_EH_BYE		(1 << 1)
#define USBIP_EH_RESET		(1 << 2)
#define USBIP_EH_UNUSABLE	(1 << 3)

#define SDEV_EVENT_REMOVED   (USBIP_EH_SHUTDOWN | USBIP_EH_RESET | USBIP_EH_BYE)
#define	SDEV_EVENT_DOWN		(USBIP_EH_SHUTDOWN | USBIP_EH_RESET)
#define	SDEV_EVENT_ERROR_TCP	(USBIP_EH_SHUTDOWN | USBIP_EH_RESET)
#define	SDEV_EVENT_ERROR_SUBMIT	(USBIP_EH_SHUTDOWN | USBIP_EH_RESET)
#define	SDEV_EVENT_ERROR_MALLOC	(USBIP_EH_SHUTDOWN | USBIP_EH_UNUSABLE)

#define	VDEV_EVENT_REMOVED	(USBIP_EH_SHUTDOWN | USBIP_EH_BYE)
#define	VDEV_EVENT_DOWN		(USBIP_EH_SHUTDOWN | USBIP_EH_RESET)
#define	VDEV_EVENT_ERROR_TCP	(USBIP_EH_SHUTDOWN | USBIP_EH_RESET)
#define	VDEV_EVENT_ERROR_MALLOC	(USBIP_EH_SHUTDOWN | USBIP_EH_UNUSABLE)

/* a common structure for stub_device and vhci_device */
struct usbip_device {
	enum usbip_device_status status;

	/* lock for status */
	pthread_mutex_t lock;

	struct usbip_sock *sock;

	unsigned long event;
	pthread_t eh;
	pthread_mutex_t eh_waitq;
	int eh_should_stop;

	struct eh_ops {
		void (*shutdown)(struct usbip_device *);
		void (*reset)(struct usbip_device *);
		void (*unusable)(struct usbip_device *);
	} eh_ops;
};

/* usbip_common.c */
void usbip_dump_trx(struct libusb_transfer *trx);
void usbip_dump_header(struct usbip_header *pdu);

int usbip_sendmsg(struct usbip_device *ud, struct kvec *vec, size_t num);
int usbip_recv(struct usbip_device *ud, void *buf, int size);

struct stub_unlink;

void usbip_pack_ret_submit(struct usbip_header *pdu,
				struct libusb_transfer *trx);
void usbip_pack_ret_unlink(struct usbip_header *pdu,
				struct stub_unlink *unlink);
void usbip_header_correct_endian(struct usbip_header *pdu, int send);

struct usbip_iso_packet_descriptor*
usbip_alloc_iso_desc_pdu(struct libusb_transfer *trx, ssize_t *bufflen);

/* some members of urb must be substituted before. */
int usbip_recv_iso(struct usbip_device *ud, struct libusb_transfer *trx);
void usbip_pad_iso(struct usbip_device *ud, struct libusb_transfer *trx);
int usbip_recv_xbuff(struct usbip_device *ud, struct libusb_transfer *trx,
			int offset);

/* usbip_event.c */
int usbip_start_eh(struct usbip_device *ud);
void usbip_stop_eh(struct usbip_device *ud);
void usbip_join_eh(struct usbip_device *ud);
void usbip_event_add(struct usbip_device *ud, unsigned long event);
int usbip_event_happened(struct usbip_device *ud);

#endif /* __STUB_COMMON_H */
