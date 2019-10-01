#ifndef __STUB_DRIVER_H__
#define __STUB_DRIVER_H__

#include <list.h>
#include <usbip_network.h>

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

/* API to be implemented by the driver */
int usbip_refresh_device_list(struct usbip_exported_devices *edevs);

int usbip_free_device_list(struct usbip_exported_devices *edevs);

struct usbip_exported_device *usbip_get_device(struct usbip_exported_devices *edevs, const char *busid);

int usbip_export_device(struct usbip_exported_device *edev, int sock_fd);

int usbip_try_transfer(struct usbip_exported_device *edev, int sock_fd);

int usbip_driver_open(void);
void usbip_driver_close(void);


#endif //__STUB_DRIVER_H__