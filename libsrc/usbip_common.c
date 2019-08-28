/*
 * Copyright (C) 2005-2007 Takahiro Hirofuchi
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
 */

#include "usbip_common.h"
#include "names.h"

#undef  PROGNAME
#define PROGNAME "libusbip"

int usbip_use_syslog;
int usbip_use_stderr;
int usbip_use_debug;

extern struct udev *udev_context;

struct speed_string {
	int num;
	char *speed;
	char *desc;
};

static const struct speed_string speed_strings[] = {
	{ USB_SPEED_UNKNOWN, "unknown", "Unknown Speed"},
	{ USB_SPEED_LOW,  "1.5", "Low Speed(1.5Mbps)"  },
	{ USB_SPEED_FULL, "12",  "Full Speed(12Mbps)" },
	{ USB_SPEED_HIGH, "480", "High Speed(480Mbps)" },
	{ USB_SPEED_WIRELESS, "53.3-480", "Wireless"},
	{ USB_SPEED_SUPER, "5000", "Super Speed(5000Mbps)" },
	{ 0, NULL, NULL }
};

struct portst_string {
	int num;
	char *desc;
};

static struct portst_string portst_strings[] = {
	{ SDEV_ST_AVAILABLE,	"Device Available" },
	{ SDEV_ST_USED,		"Device in Use" },
	{ SDEV_ST_ERROR,	"Device Error"},
	{ 0, NULL}
};

const char *usbip_status_string(int32_t status)
{
	int i;

	for (i = 0; portst_strings[i].desc != NULL; i++)
		if (portst_strings[i].num == status)
			return portst_strings[i].desc;

	return "Unknown Status";
}

const char *usbip_speed_string(int num)
{
	int i;

	for (i = 0; speed_strings[i].speed != NULL; i++)
		if (speed_strings[i].num == num)
			return speed_strings[i].desc;

	return "Unknown Speed";
}


#define DBG_UDEV_INTEGER(name)\
	dbg("%-20s = %x", to_string(name), (int) udev->name)

#define DBG_UINF_INTEGER(name)\
	dbg("%-20s = %x", to_string(name), (int) uinf->name)

void dump_usb_interface(struct usbip_usb_interface *uinf)
{
	char buff[100];

	usbip_names_get_class(buff, sizeof(buff),
			uinf->bInterfaceClass,
			uinf->bInterfaceSubClass,
			uinf->bInterfaceProtocol);
	dbg("%-20s = %s", "Interface(C/SC/P)", buff);
}

void dump_usb_device(struct usbip_usb_device *udev)
{
	char buff[100];

	dbg("%-20s = %s", "path",  udev->path);
	dbg("%-20s = %s", "busid", udev->busid);

	usbip_names_get_class(buff, sizeof(buff),
			udev->bDeviceClass,
			udev->bDeviceSubClass,
			udev->bDeviceProtocol);
	dbg("%-20s = %s", "Device(C/SC/P)", buff);

	DBG_UDEV_INTEGER(bcdDevice);

	usbip_names_get_product(buff, sizeof(buff),
			udev->idVendor,
			udev->idProduct);
	dbg("%-20s = %s", "Vendor/Product", buff);

	DBG_UDEV_INTEGER(bNumConfigurations);
	DBG_UDEV_INTEGER(bNumInterfaces);

	dbg("%-20s = %s", "speed",
			usbip_speed_string(udev->speed));

	DBG_UDEV_INTEGER(busnum);
	DBG_UDEV_INTEGER(devnum);
}


#ifndef USBIP_WITH_LIBUSB
int read_attr_value(struct udev_device *dev, const char *name,
		    const char *format)
{
	const char *attr;
	int num = 0;
	int ret;

	attr = udev_device_get_sysattr_value(dev, name);
	if (!attr) {
		err("udev_device_get_sysattr_value failed");
		goto err;
	}

	/* The client chooses the device configuration
	 * when attaching it so right after being bound
	 * to usbip-host on the server the device will
	 * have no configuration.
	 * Therefore, attributes such as bConfigurationValue
	 * and bNumInterfaces will not exist and sscanf will
	 * fail. Check for these cases and don't treat them
	 * as errors.
	 */

	ret = sscanf(attr, format, &num);
	if (ret < 1) {
		if (strcmp(name, "bConfigurationValue") &&
				strcmp(name, "bNumInterfaces")) {
			err("sscanf failed for attribute %s", name);
			goto err;
		}
	}

err:

	return num;
}


int read_attr_speed(struct udev_device *dev)
{
	const char *speed;

	speed = udev_device_get_sysattr_value(dev, "speed");
	if (!speed) {
		err("udev_device_get_sysattr_value failed");
		goto err;
	}

	for (int i = 0; speed_strings[i].speed != NULL; i++) {
		if (!strcmp(speed, speed_strings[i].speed))
			return speed_strings[i].num;
	}

err:

	return USB_SPEED_UNKNOWN;
}

#define READ_ATTR(object, type, dev, name, format)			      \
	do {								      \
		(object)->name = (type) read_attr_value(dev, to_string(name), \
							format);	      \
	} while (0)


int read_usb_device(struct udev_device *sdev, struct usbip_usb_device *udev)
{
	uint32_t busnum, devnum;
	const char *path, *name;

	READ_ATTR(udev, uint8_t,  sdev, bDeviceClass,		"%02x\n");
	READ_ATTR(udev, uint8_t,  sdev, bDeviceSubClass,	"%02x\n");
	READ_ATTR(udev, uint8_t,  sdev, bDeviceProtocol,	"%02x\n");

	READ_ATTR(udev, uint16_t, sdev, idVendor,		"%04x\n");
	READ_ATTR(udev, uint16_t, sdev, idProduct,		"%04x\n");
	READ_ATTR(udev, uint16_t, sdev, bcdDevice,		"%04x\n");

	READ_ATTR(udev, uint8_t,  sdev, bConfigurationValue,	"%02x\n");
	READ_ATTR(udev, uint8_t,  sdev, bNumConfigurations,	"%02x\n");
	READ_ATTR(udev, uint8_t,  sdev, bNumInterfaces,		"%02x\n");

	READ_ATTR(udev, uint8_t,  sdev, devnum,			"%d\n");
	udev->speed = read_attr_speed(sdev);

	path = udev_device_get_syspath(sdev);
	name = udev_device_get_sysname(sdev);

	strncpy(udev->path,  path,  SYSFS_PATH_MAX);
	strncpy(udev->busid, name, SYSFS_BUS_ID_SIZE);

	sscanf(name, "%u-%u", &busnum, &devnum);
	udev->busnum = busnum;

	return 0;
}

int read_usb_interface(struct usbip_usb_device *udev, int i,
		       struct usbip_usb_interface *uinf)
{
	char busid[SYSFS_BUS_ID_SIZE];
	struct udev_device *sif;

	sprintf(busid, "%s:%d.%d", udev->busid, udev->bConfigurationValue, i);

	sif = udev_device_new_from_subsystem_sysname(udev_context, "usb", busid);
	if (!sif) {
		err("udev_device_new_from_subsystem_sysname %s failed", busid);
		return -1;
	}

	READ_ATTR(uinf, uint8_t,  sif, bInterfaceClass,		"%02x\n");
	READ_ATTR(uinf, uint8_t,  sif, bInterfaceSubClass,	"%02x\n");
	READ_ATTR(uinf, uint8_t,  sif, bInterfaceProtocol,	"%02x\n");
	READ_ATTR(uinf, uint8_t,  sif, bInterfaceNumber,	"%02x\n");

	return 0;
}
#endif /* !USBIP_WITH_LIBUSB */

int usbip_names_init(char *f)
{
	return names_init(f);
}

void usbip_names_free(void)
{
	names_free();
}

void usbip_names_get_product(char *buff, size_t size, uint16_t vendor,
			     uint16_t product)
{
	const char *prod, *vend;

	prod = names_product(vendor, product);
	if (!prod)
		prod = "unknown product";


	vend = names_vendor(vendor);
	if (!vend)
		vend = "unknown vendor";

	snprintf(buff, size, "%s : %s (%04x:%04x)", vend, prod, vendor, product);
}

void usbip_names_get_class(char *buff, size_t size, uint8_t clazz,
			   uint8_t subclass, uint8_t protocol)
{
	const char *c, *s, *p;

	if (clazz == 0 && subclass == 0 && protocol == 0) {
		snprintf(buff, size,
			"(Defined at Interface level) (%02x/%02x/%02x)",
			clazz, subclass, protocol);
		return;
	}

	p = names_protocol(clazz, subclass, protocol);
	if (!p)
		p = "unknown protocol";

	s = names_subclass(clazz, subclass);
	if (!s)
		s = "unknown subclass";

	c = names_class(clazz);
	if (!c)
		c = "unknown class";

	snprintf(buff, size,
		"%s / %s / %s (%02x/%02x/%02x)",
		c, s, p, clazz, subclass, protocol);
}

void usbip_sock_init(struct usbip_sock *sock, int fd, void *arg,
	int (*send)(void *arg, void *buf, int len),
	int (*recv)(void *arg, void *buf, int len, int wait_all),
	void (*shutdown)(void *arg))
{
	sock->fd = fd;
	sock->arg = arg;
	sock->ux = NULL;
	sock->send = send;
	sock->recv = recv;
	sock->shutdown = shutdown;
}

struct usbip_connection_operations usbip_conn_ops = {NULL, NULL, NULL};

void usbip_conn_init(
	struct usbip_sock *(*open)(const char *host, const char *port,
				   void *opt),
	void (*close)(struct usbip_sock *sock),
	void *opt)
{
	usbip_conn_ops.open = open;
	usbip_conn_ops.close = close;
	usbip_conn_ops.opt = opt;
}

void usbip_break_all_connections(void)
{
#ifndef USBIP_WITH_LIBUSB
	usbip_ux_interrupt_pgrp();
#endif
}

void usbip_break_connection(struct usbip_sock *sock UNUSED)
{
#ifndef USBIP_WITH_LIBUSB
	usbip_ux_interrupt(sock);
#endif
}

void usbip_set_use_debug(int val)
{
	usbip_use_debug = val;
}

void usbip_set_use_stderr(int val)
{
	usbip_use_stderr = val;
}

void usbip_set_use_syslog(int val)
{
	usbip_use_syslog = val;
}
