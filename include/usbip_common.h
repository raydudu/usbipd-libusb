/*
 * Copyright (C) 2005-2007 Takahiro Hirofuchi
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
 */

#ifndef __USBIP_COMMON_H
#define __USBIP_COMMON_H

#include "usbip_config.h"

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <linux/usb/ch9.h>
#include <usbip.h>
#endif



#define SYSFS_PATH_MAX		256
#define SYSFS_BUS_ID_SIZE	32
#ifndef SYSFS_NAME_LEN
#define SYSFS_NAME_LEN          64
#endif

extern int usbip_use_debug;

#define pr_fmt(fmt)	"%s: %s: " fmt "\n", PACKAGE

#ifndef USBIP_OS_NO_NR_ARGS
#define dbg_fmt(fmt)	pr_fmt("%s:%d:[%s] " fmt), "debug",	\
		        __FILE__, __LINE__, __func__

#define err(fmt, args...)						\
	do {								\
			fprintf(stderr, pr_fmt(fmt), "error", ##args);	\
	} while (0)

#define info(fmt, args...)						\
	do {								\
			fprintf(stderr, pr_fmt(fmt), "info", ##args);	\
	} while (0)

#define dbg(fmt, args...)						\
	do {								\
	if (usbip_use_debug) {						\
			fprintf(stderr, dbg_fmt(fmt), ##args);		\
	}								\
	} while (0)
#endif /* !USBIP_OS_NO_NR_ARGS */


struct usbip_usb_interface {
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t bInterfaceNumber;
} __attribute__((__packed__));

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

#define to_string(s)	#s

void dump_usb_interface(struct usbip_usb_interface *);
void dump_usb_device(struct usbip_usb_device *);

const char *usbip_speed_string(int num);

void usbip_names_get_product(char *buff, size_t size, uint16_t vendor,
			     uint16_t product);
void usbip_names_get_class(char *buff, size_t size, uint8_t clazz,
			   uint8_t subclass, uint8_t protocol);

#endif /* __USBIP_COMMON_H */
