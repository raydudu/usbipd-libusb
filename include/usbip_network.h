/*
 * Copyright (C) 2005-2007 Takahiro Hirofuchi
 */

#ifndef __USBIP_NETWORK_H
#define __USBIP_NETWORK_H

#include "usbip_config.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <stdint.h>

extern int usbip_port;
extern char *usbip_port_string;
void usbip_setup_port_number(char *arg);

#ifdef __linux__
#include <linux/usb/ch9.h>
#elif __APPLE__
// Copied from linux/usb/ch9.h
enum usb_device_speed {
    USB_SPEED_UNKNOWN = 0,                  /* enumerating */
    USB_SPEED_LOW, USB_SPEED_FULL,          /* usb 1.1 */
    USB_SPEED_HIGH,                         /* usb 2.0 */
    USB_SPEED_WIRELESS,                     /* wireless (usb 2.5) */
    USB_SPEED_SUPER,                        /* usb 3.0 */
};
#endif

#include <usbip.h>

#define SYSFS_PATH_MAX		256
#define SYSFS_BUS_ID_SIZE	32

struct usbip_usb_interface {
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t bInterfaceNumber; //TODO in original implementation: uint8_t padding;	/* alignment */
} __attribute__((packed));

struct usbip_usb_device {
    char path[SYSFS_PATH_MAX];
    char busid[SYSFS_BUS_ID_SIZE];

    uint32_t busnum;
    uint32_t devnum;
    uint32_t speed;

    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;

    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bConfigurationValue;
    uint8_t bNumConfigurations;
    uint8_t bNumInterfaces;
} __attribute__((packed));


/* ---------------------------------------------------------------------- */
/* Common header for all the kinds of PDUs. */
struct op_common {
	uint16_t version;
	uint16_t code;
	uint32_t status; /* op_code status (for reply) */
} __attribute__((packed));

#define OP_REQUEST	(0x80 << 8)
#define OP_REPLY	(0x00 << 8)

/* add more error code */
#define ST_OK			0x00
#define ST_NA			0x01
#define ST_NO_FREE_PORT		0x02
#define ST_DEVICE_NOT_FOUND	0x03

/* ---------------------------------------------------------------------- */
/* Dummy Code */
#define OP_UNSPEC	0x00

/* ---------------------------------------------------------------------- */
/* Import a remote USB device. */
#define OP_IMPORT	0x03
#define OP_REQ_IMPORT	(OP_REQUEST | OP_IMPORT)
#define OP_REP_IMPORT   (OP_REPLY   | OP_IMPORT)

struct op_import_request {
	char busid[SYSFS_BUS_ID_SIZE];
} __attribute__((packed));


#define PACK_OP_IMPORT_REQUEST(pack, request)  do {\
} while (0)

#define PACK_OP_IMPORT_REPLY(pack, reply)  do {\
    if (pack) { \
        reply.busnum = htonl(reply.busnum); \
        reply.devnum = htonl(reply.devnum); \
        reply.speed = htonl(reply.speed); \
        reply.idVendor = htons(reply.idVendor); \
        reply.idProduct = htons(reply.idProduct); \
        reply.bcdDevice = htons(reply.bcdDevice); \
    } else { \
        reply.busnum = ntohl(reply.busnum); \
        reply.devnum = ntohl(reply.devnum); \
        reply.speed = ntohl(reply.speed); \
        reply.idVendor = ntohs(reply.idVendor); \
        reply.idProduct = ntohs(reply.idProduct); \
        reply.bcdDevice = ntohs(reply.bcdDevice); \
    } \
} while (0)


/* ---------------------------------------------------------------------- */
/* Retrieve the list of exported USB devices. */
#define OP_DEVLIST	0x05
#define OP_REQ_DEVLIST	(OP_REQUEST | OP_DEVLIST)
#define OP_REP_DEVLIST	(OP_REPLY   | OP_DEVLIST)

struct op_devlist_reply {
	uint32_t ndev;
	/* followed by op_devlist_reply_extra[] */
} __attribute__((packed));

/*
struct op_devlist_reply_extra {
	struct usbip_usb_device    udev;
	struct usbip_usb_interface uinf[];
} __attribute__((packed));
*/


ssize_t usbip_net_recv(int sock_fd, void *buff, size_t bufflen);
ssize_t usbip_net_send(int sock_fd, void *buff, size_t bufflen);
int usbip_net_sendvec(int fd, struct iovec *vec, size_t num);
//int usbip_recv(struct usbip_device *ud, void *buf, int size);


int usbip_net_send_op_common(int sock_fd, uint32_t code, uint32_t status);
int usbip_net_recv_op_common(int sock_fd, uint16_t *code);
int usbip_net_set_reuseaddr(int sockfd);
int usbip_net_set_nodelay(int sockfd);
int usbip_net_set_keepalive(int sockfd);
int usbip_net_set_v6only(int sockfd);
const char *usbip_net_gai_strerror(int errcode);



#endif /* __USBIP_NETWORK_H */
