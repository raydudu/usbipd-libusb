/*
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#include "usbip_host_common.h"

static int dummy_driver_open(void)
{
	info("dummy_driver_open");
	return -1;
}

static void dummy_driver_close(void)
{
	info("dummy_driver_close");
}

static int dummy_refresh_device_list(struct usbip_exported_devices *edevs)
{
	info("dummy_refresh_device_list %p", edevs);
	return -1;
}

static void dummy_free_device_list(struct usbip_exported_devices *edevs)
{
	info("dummy_free_device_list %p", edevs);
}

static struct usbip_exported_device *dummy_get_device(
			struct usbip_exported_devices *edevs,
			const char *busid)
{
	info("dummy_get_device %p %s", edevs, busid);
	return NULL;
}

static int dummy_list_devices(struct usbip_usb_device **udevs)
{
	info("dummy_get_device %p", udevs);
	return -1;
}

static int dummy_export_device(struct usbip_exported_device *edev,
			       struct usbip_sock *sock)
{
	info("dummy_get_device %p %p", edev, sock);
	return -1;
}

/* do not use gcc initialization syntax for other compilers */
struct usbip_host_driver device_driver = {
	"dummy-device",
	{
		dummy_driver_open,
		dummy_driver_close,
		dummy_refresh_device_list,
		dummy_free_device_list,
		dummy_get_device,
		dummy_list_devices,
		NULL, /* bind */
		NULL, /* unbind */
		dummy_export_device,
		NULL, /* transfer */
		NULL, /* has_transferred */
		NULL, /* read_device */
		NULL, /* read_interface */
		NULL  /* is_my_device */
	},
};
