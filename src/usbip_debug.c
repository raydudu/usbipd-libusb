#include <stdio.h>
#include <netinet/in.h>

#include "names.h"

#include <usbip_config.h>
#include <usbip_debug.h>


int usbip_use_debug;

#ifdef CONFIG_USBIP_DEBUG
unsigned long usbip_stub_debug_flags;
#else
unsigned long usbip_stub_debug_flags;
#endif

#define to_string(s)	#s

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
                 "(Defined at Interface level) (%02x/%02x/%02x)", clazz, subclass, protocol);
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

    snprintf(buff, size, "%s / %s / %s (%02x/%02x/%02x)", c, s, p, clazz, subclass, protocol);
}


void usbip_dump_buffer(void *buf, int size){
    unsigned i, j, k;
    unsigned char *pbuf = buf;

    for (i=0; i<size; i+=16) {
        printf("\n  %08x  ", i);
        for(j=0,k=0; k<16; j++,k++) {
            if (i+j < size) {
                printf("%02x", pbuf[i + j]);
            } else {
                printf("  ");
            }
            printf(" ");
        }
        printf(" ");
        for(j=0,k=0; k<16; j++,k++) {
            if (i+j < size) {
                if ((pbuf[i + j] < 32) || (pbuf[i + j] > 126)) {
                    printf(".");
                } else {
                    printf("%c", pbuf[i + j]);
                }
            }
        }
    }
    printf("\n" );
}