/* SPDX-License-Identifier: GPL-2.0 */
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

#define to_string(s)	#s

void dump_usb_interface(struct usbip_usb_interface *);
void dump_usb_device(struct usbip_usb_device *);

const char *usbip_speed_string(int num);

void usbip_names_get_product(char *buff, size_t size, uint16_t vendor,
			     uint16_t product);
void usbip_names_get_class(char *buff, size_t size, uint8_t clazz,
			   uint8_t subclass, uint8_t protocol);

#endif /* __USBIP_COMMON_H */
