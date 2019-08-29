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

#include <sys/stat.h>

#include "stub.h"

#include <usbip_debug.h>

libusb_context *stub_libusb_ctx;

static struct stub_endpoint *get_endpoint(struct stub_device *sdev, uint8_t ep)
{
	int i;
	uint8_t ep_nr = ep & USB_ENDPOINT_NUMBER_MASK;

	for (i = 0; i < sdev->num_eps; i++) {
		if ((sdev->eps + i)->nr == ep_nr)
			return sdev->eps + i;
	}
	return NULL;
}

uint8_t stub_get_transfer_type(struct stub_device *sdev, uint8_t ep)
{
	struct stub_endpoint *epp;

	if (ep == 0)
		return LIBUSB_TRANSFER_TYPE_CONTROL;

	epp = get_endpoint(sdev, ep);
	if (epp == NULL) {
		dbg("Unknown endpoint %d", ep);
		return 0xff;
	}
	return epp->type;
}

uint8_t stub_endpoint_dir(struct stub_device *sdev, uint8_t ep)
{
	struct stub_endpoint *epp;

	epp = get_endpoint(sdev, ep);
	if (epp == NULL) {
		dbg("Direction for %d is undetermined", ep);
		return 0;
	}
	return epp->dir;
}

int stub_endpoint_dir_out(struct stub_device *sdev, uint8_t ep)
{
	uint8_t dir = stub_endpoint_dir(sdev, ep);

	if (dir == LIBUSB_ENDPOINT_OUT)
		return 1;
	return 0;
}

uint8_t stub_get_transfer_flags(uint32_t in)
{
	uint8_t flags = 0;

	if (in & URB_SHORT_NOT_OK)
		flags |= LIBUSB_TRANSFER_SHORT_NOT_OK;
	if (in & URB_ZERO_PACKET)
		flags |= LIBUSB_TRANSFER_ADD_ZERO_PACKET;

	/*
	 * URB_FREE_BUFFER is turned off to free by stub_free_priv_and_trx()
	 *
	 * URB_ISO_ASAP, URB_NO_TRANSFER_DMA_MAP, URB_NO_FSBR and
	 * URB_NO_INTERRUPT are ignored because unsupported by libusb.
	 */
	return flags;
}

int usbip_driver_open(void) {
	return libusb_init(&stub_libusb_ctx);
}

void usbip_driver_close(void) {
	libusb_exit(stub_libusb_ctx);
}

static void get_busid(libusb_device *dev, char *buf)
{
	snprintf(buf, SYSFS_BUS_ID_SIZE, "%d-%d",
		libusb_get_bus_number(dev),
		libusb_get_device_address(dev));
}

static uint32_t get_device_speed(libusb_device *dev)
{
	int speed = libusb_get_device_speed(dev);

	switch (speed) {
	case LIBUSB_SPEED_LOW:
		return USB_SPEED_LOW;
	case LIBUSB_SPEED_FULL:
		return USB_SPEED_FULL;
	case LIBUSB_SPEED_HIGH:
		return USB_SPEED_HIGH;
	case LIBUSB_SPEED_SUPER:
        return USB_SPEED_SUPER;
	default:
		dbg("unknown speed enum %d", speed);
	}
	return USB_SPEED_UNKNOWN;
}

static void __fill_usb_device(struct usbip_usb_device *udev,
				libusb_device *dev,
				struct libusb_device_descriptor *desc,
				struct libusb_config_descriptor *config)
{
	char busid[SYSFS_BUS_ID_SIZE];

	get_busid(dev, busid);

	memset((char *)udev, 0, sizeof(struct usbip_usb_device));
	strncpy(udev->busid, busid, SYSFS_BUS_ID_SIZE);
	udev->busnum = libusb_get_bus_number(dev);
	udev->devnum = libusb_get_device_address(dev);
	udev->speed = get_device_speed(dev);
	udev->idVendor = desc->idVendor;
	udev->idProduct = desc->idProduct;
	udev->bcdDevice = desc->bcdDevice;
	udev->bDeviceClass = desc->bDeviceClass;
	udev->bDeviceSubClass = desc->bDeviceSubClass;
	udev->bDeviceProtocol = desc->bDeviceProtocol;
	udev->bConfigurationValue = config->bConfigurationValue;
	udev->bNumConfigurations = desc->bNumConfigurations;
	udev->bNumInterfaces = config->bNumInterfaces;
}

#define FILL_SKIPPED 1

static int fill_usb_device(struct usbip_usb_device *udev, libusb_device *dev)
{
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *config;
	char busid[SYSFS_BUS_ID_SIZE];

	if (libusb_get_device_descriptor(dev, &desc)) {
		get_busid(dev, busid);
		err("get device desc %s", busid);
		return -1;
	}

	if (desc.bDeviceClass == USB_CLASS_HUB) {
		get_busid(dev, busid);
		dbg("skip hub to fill udev %s", busid);
		return FILL_SKIPPED;
	}

	if (libusb_get_active_config_descriptor(dev, &config)) {
		get_busid(dev, busid);
		err("get device config %s", busid);
		return -1;
	}

	__fill_usb_device(udev, dev, &desc, config);

	libusb_free_config_descriptor(config);
	return 0;
}

static void fill_usb_interfaces(struct usbip_usb_interface *uinf,
				struct libusb_config_descriptor *config)
{
	int i;
	const struct libusb_interface_descriptor *intf;

	for (i = 0; i < config->bNumInterfaces; i++) {
		intf = (config->interface + i)->altsetting;
		(uinf + i)->bInterfaceClass = intf->bInterfaceClass;
		(uinf + i)->bInterfaceSubClass = intf->bInterfaceSubClass;
		(uinf + i)->bInterfaceProtocol = intf->bInterfaceProtocol;
		(uinf + i)->bInterfaceNumber = intf->bInterfaceNumber;
	}
}

static void fill_stub_endpoint(struct stub_endpoint *ep,
			       const struct libusb_endpoint_descriptor *desc)
{
	ep->nr = desc->bEndpointAddress & LIBUSB_ENDPOINT_ADDRESS_MASK;
	ep->dir = desc->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK;
	ep->type = desc->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK;
}

static void fill_stub_endpoints(struct stub_endpoint *ep,
				struct libusb_config_descriptor *config)
{
	int i, j, k, num = 0;
	const struct libusb_interface *intf;
	const struct libusb_interface_descriptor *idesc;

	for (i = 0; i < config->bNumInterfaces; i++) {
		intf = config->interface + i;
		for (j = 0; j < intf->num_altsetting; j++) {
			idesc = intf->altsetting + j;
			for (k = 0; k < idesc->bNumEndpoints; k++) {
				fill_stub_endpoint(ep + num,
						   idesc->endpoint + k);
				num++;
			}
		}
	}
}

static inline
int edev2num_ifs(struct usbip_exported_device *edev)
{
	return edev->udev.bNumInterfaces;
}

static inline
struct stub_edev_data *edev2edev_data(struct usbip_exported_device *edev)
{
	return (struct stub_edev_data *)(edev->uinf + edev2num_ifs(edev));
}

static inline
int edev2num_eps(struct usbip_exported_device *edev)
{
	struct stub_edev_data *edev_data = edev2edev_data(edev);

	return edev_data->num_eps;
}

static int count_endpoints(struct libusb_config_descriptor *config)
{
	int i, j, num = 0;
	const struct libusb_interface *intf;
	const struct libusb_interface_descriptor *idesc;

	for (i = 0; i < config->bNumInterfaces; i++) {
		intf = config->interface + i;
		for (j = 0; j < intf->num_altsetting; j++) {
			idesc = intf->altsetting + j;
			num += idesc->bNumEndpoints;
		}
	}
	return num;
}

static struct usbip_exported_device *exported_device_new(
				libusb_device *dev,
				struct libusb_device_descriptor *desc)
{
	struct libusb_config_descriptor *config;
	struct usbip_exported_device *edev;
	struct stub_edev_data *edev_data;
	int num_eps;

	if (libusb_get_active_config_descriptor(dev, &config)) {
		err("get device config");
		goto err_out;
	}

	num_eps = count_endpoints(config);

	edev = (struct usbip_exported_device *)calloc(1,
			sizeof(struct usbip_exported_device) +
			(config->bNumInterfaces *
				sizeof(struct usbip_usb_interface)) +
			sizeof(struct stub_edev_data) +
			(num_eps *
				sizeof(struct stub_endpoint)));
	if (!edev) {
		err("alloc edev");
		goto err_free_config;
	}
	edev_data =
		(struct stub_edev_data *)(edev->uinf + config->bNumInterfaces);

	__fill_usb_device(&edev->udev, dev, desc, config);
	fill_usb_interfaces(edev->uinf, config);
	edev_data = edev2edev_data(edev);
	edev_data->dev = dev;
	edev_data->num_eps = num_eps;
	fill_stub_endpoints(edev_data->eps, config);

	libusb_free_config_descriptor(config);
	return edev;

err_free_config:
	libusb_free_config_descriptor(config);
err_out:
	return NULL;
}

static void stub_device_delete(struct stub_device *sdev);

static void exported_device_delete(struct usbip_exported_device *edev)
{
	struct stub_edev_data *edev_data = edev2edev_data(edev);

	if (edev_data->sdev)
		stub_device_delete(edev_data->sdev);
	free(edev);
}

int usbip_refresh_device_list(struct usbip_exported_devices *edevs) {
	int num, i;
	libusb_device **devs, *dev;
	struct usbip_exported_device *edev;
	struct libusb_device_descriptor desc;
	char busid[SYSFS_BUS_ID_SIZE];

	edevs->ndevs = 0;
	INIT_LIST_HEAD(&edevs->edev_list);
	edevs->data = NULL;

	num = libusb_get_device_list(stub_libusb_ctx, &devs);
	if (num < 0) {
		err("get device list");
		goto err_out;
	}

	for (i = 0; i < num; i++) {
		dev = *(devs + i);
		get_busid(dev, busid);
/*
		if (!stub_find_exported_device(busid))
			continue;
*/
		if (libusb_get_device_descriptor(dev, &desc)) {
			err("get device desc %s", busid);
			goto err_free_list;
		}
		if (desc.bDeviceClass == USB_CLASS_HUB) {
			dbg("skip hub to fill udev %s", busid);
			continue;
		}
		edev = exported_device_new(dev, &desc);
		if (!edev)
			goto err_out;
		list_add(&edev->node, &edevs->edev_list);
		edevs->ndevs++;
	}

	edevs->data = (void *)devs;
	return 0;

err_free_list:
	libusb_free_device_list(devs, 1);
err_out:
	return -1;
}

int usbip_free_device_list(struct usbip_exported_devices *edevs) {
	libusb_device **devs = (libusb_device **)edevs->data;
	struct list_head *i, *tmp;
	struct usbip_exported_device *edev;

	if (devs)
		libusb_free_device_list(devs, 1);

	list_for_each_safe(i, tmp, &edevs->edev_list) {
		edev = list_entry(i, struct usbip_exported_device, node);
		list_del(i);
		exported_device_delete(edev);
	}
    return 0;
}

struct usbip_exported_device *usbip_get_device(struct usbip_exported_devices *edevs, const char *busid) {
	struct list_head *i;
	struct usbip_exported_device *edev;

	list_for_each(i, &edevs->edev_list) {
		edev = list_entry(i, struct usbip_exported_device, node);
		if (!strncmp(busid, edev->udev.busid, SYSFS_BUS_ID_SIZE))
			return edev;
	}

	return NULL;
}

static void stub_shutdown(struct usbip_device *ud)
{
	struct stub_device *sdev = container_of(ud, struct stub_device, ud);

	sdev->should_stop = 1;
	usbip_stop_eh(&sdev->ud);
	pthread_mutex_unlock(&sdev->tx_waitq);
	/* rx will exit by disconnect */
}

static void stub_device_reset(struct usbip_device *ud)
{
	struct stub_device *sdev = container_of(ud, struct stub_device, ud);
	int ret;

	dev_dbg(sdev->dev, "device reset");

	/* try to reset the device */
	ret = libusb_reset_device(sdev->dev_handle);

	pthread_mutex_lock(&ud->lock);
	if (ret) {
		dev_err(sdev->dev, "device reset");
		ud->status = SDEV_ST_ERROR;
	} else {
		dev_info(sdev->dev, "device reset");
		ud->status = SDEV_ST_AVAILABLE;
	}
	pthread_mutex_unlock(&ud->lock);
}

static void stub_device_unusable(struct usbip_device *ud)
{
	pthread_mutex_lock(&ud->lock);
	ud->status = SDEV_ST_ERROR;
	pthread_mutex_unlock(&ud->lock);
}

static void init_usbip_device(struct usbip_device *ud)
{
	ud->status = SDEV_ST_AVAILABLE;
	pthread_mutex_init(&ud->lock, NULL);

	ud->eh_ops.shutdown = stub_shutdown;
	ud->eh_ops.reset    = stub_device_reset;
	ud->eh_ops.unusable = stub_device_unusable;
}

static void clear_usbip_device(struct usbip_device *ud)
{
	pthread_mutex_destroy(&ud->lock);
}

static inline uint32_t get_devid(libusb_device *dev)
{
	uint32_t bus_number = libusb_get_bus_number(dev);
	uint32_t dev_addr = libusb_get_device_address(dev);

	return (bus_number << 16) | dev_addr;
}

static inline struct stub_device *edev2sdev(struct usbip_exported_device *edev)
{
	struct stub_edev_data *edev_data = edev2edev_data(edev);

	return edev_data->sdev;
}

static struct stub_device *stub_device_new(struct usbip_exported_device *edev)
{
	struct stub_device *sdev;
	struct stub_edev_data *edev_data = edev2edev_data(edev);
	int num_ifs = edev2num_ifs(edev);
	int num_eps = edev2num_eps(edev);
	int i;

	sdev = (struct stub_device *)calloc(1,
			sizeof(struct stub_device) +
			(sizeof(struct stub_interface) * num_ifs) +
			(sizeof(struct stub_endpoint) * num_eps));
	if (!sdev) {
		err("alloc sdev");
		return NULL;
	}

	sdev->dev = edev_data->dev;
	memcpy(&sdev->udev, &edev->udev, sizeof(struct usbip_usb_device));
	init_usbip_device(&sdev->ud);
	sdev->devid = get_devid(edev_data->dev);
	for (i = 0; i < num_ifs; i++)
		memcpy(&((sdev->ifs + i)->uinf), edev->uinf + i,  sizeof(struct usbip_usb_interface));
	sdev->num_eps = num_eps;

	sdev->eps = (struct stub_endpoint *)(sdev->ifs + num_ifs);
	for (i = 0; i < num_eps; i++)
		memcpy(sdev->eps + i, edev_data->eps + i,
			sizeof(struct stub_endpoint));

	pthread_mutex_init(&sdev->priv_lock, NULL);
	INIT_LIST_HEAD(&sdev->priv_init);
	INIT_LIST_HEAD(&sdev->priv_tx);
	INIT_LIST_HEAD(&sdev->priv_free);
	INIT_LIST_HEAD(&sdev->unlink_tx);
	INIT_LIST_HEAD(&sdev->unlink_free);
	pthread_mutex_init(&sdev->tx_waitq, NULL);
	pthread_mutex_lock(&sdev->tx_waitq);

	return sdev;
}

static void stub_device_delete(struct stub_device *sdev)
{
	clear_usbip_device(&sdev->ud);
	pthread_mutex_destroy(&sdev->priv_lock);
	pthread_mutex_destroy(&sdev->tx_waitq);
	free(sdev);
}

static void release_interface(libusb_device_handle *dev_handle,
			      struct stub_interface *intf, int force)
{
	int nr = intf->uinf.bInterfaceNumber;
	int ret;

	if (force || intf->claimed) {
		ret = libusb_release_interface(dev_handle, nr);
		if (ret == 0)
			intf->claimed = 0;
		else
			dbg("failed to release interface %d by %d", nr, ret);
	}
	if (force || intf->detached) {
		ret = libusb_attach_kernel_driver(dev_handle, nr);
		if (ret == 0)
			intf->detached = 0;
		else
			dbg("failed to attach interface %d by %d", nr, ret);
	}
}

static void release_interfaces(libusb_device_handle *dev_handle, int num_ifs,
			       struct stub_interface *intfs, int force)
{
	int i;

	for (i = 0; i < num_ifs; i++)
		release_interface(dev_handle, intfs + i, force);
}

static int claim_interface(libusb_device_handle *dev_handle,
			   struct stub_interface *intf)
{
	int nr = intf->uinf.bInterfaceNumber;
	int ret;

	ret = libusb_detach_kernel_driver(dev_handle, nr);
	if (!(ret == 0 || ret == LIBUSB_ERROR_NOT_FOUND)) {
		dbg("failed to detach interface %d by %d", nr, ret);
		/* ignore error, because some platform doesn't support */
	} else {
        intf->detached = 1;
    }

	dbg("claiming interface %d", nr);
	ret = libusb_claim_interface(dev_handle, nr);
	if (ret) {
		dbg("failed to claim interface %d by %d", nr, ret);
		release_interface(dev_handle, intf, 0);
		return -1;
	}
	intf->claimed = 1;
	return 0;
}

static int claim_interfaces(libusb_device_handle *dev_handle, int num_ifs,
			    struct stub_interface *intfs)
{
	int i;

	for (i = 0; i < num_ifs; i++) {
		if (claim_interface(dev_handle, intfs + i))
			return -1;
	}
	return 0;
}
int usbip_export_device(struct usbip_exported_device *edev, int sock_fd) {
	struct stub_device *sdev;
	struct stub_edev_data *edev_data = edev2edev_data(edev);
	int ret;

	sdev = stub_device_new(edev);
	if (!sdev)
		goto err_out;

	edev_data->sdev = sdev;

	ret = libusb_open(sdev->dev, &sdev->dev_handle);
	if (ret) {
		if (ret == LIBUSB_ERROR_ACCESS)
			err("access denied to open device %s",
					edev->udev.busid);
		else
			err("failed to open device %s by %d",
					edev->udev.busid, ret);

		goto err_out;
	}

	if (claim_interfaces(sdev->dev_handle, sdev->udev.bNumInterfaces,
			     sdev->ifs)) {
		err("claim interfaces %s", edev->udev.busid);
		goto err_close_lib;
	}

	sdev->ud.sock_fd = sock_fd;

	return 0;

err_close_lib:
	libusb_close(sdev->dev_handle);
	sdev->dev_handle = NULL;
err_out:
	return -1;
}

void stub_unexport_device(struct stub_device *sdev)
{
	release_interfaces(sdev->dev_handle, sdev->udev.bNumInterfaces,
			   sdev->ifs, 0);
	libusb_close(sdev->dev_handle);
	sdev->dev_handle = NULL;
}

static int stub_start(struct stub_device *sdev)
{
	if (sdev == NULL)
		return 0;

	if (usbip_start_eh(&sdev->ud)) {
		err("start event handler");
		return -1;
	}
	if (pthread_create(&sdev->rx, NULL, stub_rx_loop, sdev)) {
		err("start recv thread");
		return -1;
	}
	if (pthread_create(&sdev->tx, NULL, stub_tx_loop, sdev)) {
		err("start send thread");
		return -1;
	}
	pthread_mutex_lock(&sdev->ud.lock);
	sdev->ud.status = SDEV_ST_USED;
	pthread_mutex_unlock(&sdev->ud.lock);
	dbg("successfully started libusb transmission");
	return 0;
}

static void stub_join(struct stub_device *sdev)
{
	if (sdev == NULL)
		return;
	dbg("waiting on libusb transmission threads");
	usbip_join_eh(&sdev->ud);
	pthread_join(sdev->tx, NULL);
	pthread_join(sdev->rx, NULL);
}

int usbip_try_transfer(struct usbip_exported_device *edev, int sock_fd) {
	struct stub_device *sdev = edev2sdev(edev);

	if (stub_start(sdev)) {
		err("start driver-libusb");
		return -1;
	}
	stub_join(sdev);
	stub_device_cleanup_transfers(sdev);
	stub_device_cleanup_unlinks(sdev);
	stub_unexport_device(sdev);

	return 0;
}


