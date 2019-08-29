#include <stdio.h>

#include <usbip_config.h>

#include <usbip_debug.h>

int usbip_use_debug;

#ifdef CONFIG_USBIP_DEBUG
unsigned long usbip_stub_debug_flags = 0xffffffff;
#else
unsigned long usbip_stub_debug_flags;
#endif


void usbip_dump_buffer(char *buff, int bufflen) {
    unsigned char *p = (unsigned char *) buff;
    int rem = bufflen;
    int i;
    struct b {
        char b[3];
    };
    struct b a[16];

    while (rem > 0) {
        for (i = 0; i < 16; i++) {
            if (i < rem)
                sprintf(a[i].b, "%02x", *(p + i));
            else
                a[i].b[0] = 0;
        }
        fprintf(stdout,
                "%s %s %s %s %s %s %s %s  %s %s %s %s %s %s %s %s\n",
                a[0].b, a[1].b, a[2].b, a[3].b,
                a[4].b, a[5].b, a[6].b, a[7].b,
                a[8].b, a[9].b, a[10].b, a[11].b,
                a[12].b, a[13].b, a[14].b, a[15].b);
        if (rem > 16) {
            rem -= 16;
            p += 16;
        } else {
            rem = 0;
        }
    }
}