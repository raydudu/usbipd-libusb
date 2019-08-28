/*
 * Copyright (C) 2015-2016 Samsung Electronics
 *               Igor Kotrasinski <i.kotrasinsk@samsung.com>
 *               Krzysztof Opasiak <k.opasiak@samsung.com>
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
 *
 * Refactored from usbip_host_driver.c, which is:
 * Copyright (C) 2011 matt mooney <mfm@muteddisk.com>
 *               2005-2007 Takahiro Hirofuchi
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

#ifndef __USBIP_HOST_COMMON_H
#define __USBIP_HOST_COMMON_H

#include "usbip_config.h"

#include <stdint.h>
#ifdef USBIP_OS_NO_LIBUDEV
struct udev_device {
	void *p;
};
#else
#include <libudev.h>
#endif
#include <errno.h>
#include "list.h"
#include "usbip_common.h"

extern struct udev *udev_context;

struct usbip_host_driver;
struct usbip_exported_devices;

struct usbip_host_driver_ops {
	int (*open)(void);
	void (*close)(void);
	int (*refresh_device_list)(struct usbip_exported_devices *edevs);
	void (*free_device_list)(struct usbip_exported_devices *edevs);
	struct usbip_exported_device * (*get_device)(
			struct usbip_exported_devices *edevs,
			const char *busid);

	int (*list_devices)(struct usbip_usb_device **udevs);
	int (*bind_device)(const char *busid);
	int (*unbind_device)(const char *busid);
	int (*export_device)(struct usbip_exported_device *edev,
			struct usbip_sock *sock);
	int (*try_transfer)(struct usbip_exported_device *edev,
			struct usbip_sock *sock);
	int (*has_transferred)(void);

	int (*read_device)(struct udev_device *sdev,
			   struct usbip_usb_device *dev);
	int (*read_interface)(struct usbip_usb_device *udev, int i,
			      struct usbip_usb_interface *uinf);
	int (*is_my_device)(struct udev_device *udev);
};

struct usbip_host_driver {
	const char *udev_subsystem;
	struct usbip_host_driver_ops ops;
};

extern struct usbip_host_driver *usbip_hdriver;

struct usbip_exported_devices {
	int ndevs;
	struct list_head edev_list;
	void *data;
};

struct usbip_exported_device {
	//struct udev_device *sudev;
	int32_t status;
	struct usbip_usb_device udev;
	struct list_head node;
	struct usbip_usb_interface uinf[];
};

/* External API to access the driver */
static inline int usbip_driver_open(void)
{
	if (!usbip_hdriver->ops.open)
		return -EOPNOTSUPP;
	return usbip_hdriver->ops.open();
}

static inline void usbip_driver_close(void)
{
	if (!usbip_hdriver->ops.close)
		return;
	usbip_hdriver->ops.close();
}

static inline int usbip_refresh_device_list(
			struct usbip_exported_devices *edevs)
{
	if (!usbip_hdriver->ops.refresh_device_list)
		return -EOPNOTSUPP;
	return usbip_hdriver->ops.refresh_device_list(edevs);
}

static inline int usbip_free_device_list(
			struct usbip_exported_devices *edevs)
{
	if (!usbip_hdriver->ops.refresh_device_list)
		return -EOPNOTSUPP;
	return usbip_hdriver->ops.refresh_device_list(edevs);
}

static inline struct usbip_exported_device *usbip_get_device(
			struct usbip_exported_devices *edevs,
			const char *busid)
{
	if (!usbip_hdriver->ops.get_device)
		return NULL;
	return usbip_hdriver->ops.get_device(edevs, busid);
}

static inline int usbip_list_devices(struct usbip_usb_device **udevs)
{
	if (!usbip_hdriver->ops.list_devices)
		return -1;
	return usbip_hdriver->ops.list_devices(udevs);
}

static inline int usbip_export_device(struct usbip_exported_device *edev,
				      struct usbip_sock *sock)
{
	if (!usbip_hdriver->ops.export_device)
		return -1;
	return usbip_hdriver->ops.export_device(edev, sock);
}

static inline int usbip_try_transfer(struct usbip_exported_device *edev,
				     struct usbip_sock *sock)
{
	if (!usbip_hdriver->ops.try_transfer)
		return -1;
	return usbip_hdriver->ops.try_transfer(edev, sock);
}

static inline int usbip_has_transferred(void)
{
	if (!usbip_hdriver->ops.has_transferred)
		return -1;
	return usbip_hdriver->ops.has_transferred();
}


#endif /* __USBIP_HOST_COMMON_H */
