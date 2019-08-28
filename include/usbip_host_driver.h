#ifndef __STUB_DRIVER_H__
#define __STUB_DRIVER_H__

#include <list.h>

struct usbip_exported_devices {
    int ndevs;
    struct list_head edev_list;
    void *data;
};

struct usbip_exported_device {
    int32_t status;
    struct usbip_usb_device udev;
    struct list_head node;
    struct usbip_usb_interface uinf[];
};

int usbip_driver_open(void);
//    return usbip_hdriver->ops.open();

void usbip_driver_close(void);
//    usbip_hdriver->ops.close();

int usbip_refresh_device_list(struct usbip_exported_devices *edevs);
//    return usbip_hdriver->ops.refresh_device_list(edevs);

int usbip_free_device_list(struct usbip_exported_devices *edevs);
//    return usbip_hdriver->ops.refresh_device_list(edevs);

struct usbip_exported_device *usbip_get_device(struct usbip_exported_devices *edevs, const char *busid);
//    return usbip_hdriver->ops.get_device(edevs, busid);

int usbip_list_devices(struct usbip_usb_device **udevs);
//    return usbip_hdriver->ops.list_devices(udevs);

int usbip_export_device(struct usbip_exported_device *edev, struct usbip_sock *sock);
//    return usbip_hdriver->ops.export_device(edev, sock);

int usbip_try_transfer(struct usbip_exported_device *edev, struct usbip_sock *sock);
//    return usbip_hdriver->ops.try_transfer(edev, sock);

int usbip_has_transferred(void);
//    return usbip_hdriver->ops.has_transferred();

#endif //__STUB_DRIVER_H__