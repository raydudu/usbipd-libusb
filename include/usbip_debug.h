#ifndef __USBIP_DEBUG_H__
#define __USBIP_DEBUG_H__

#include <libusb.h>
#include <stdio.h>

extern int usbip_use_debug;
extern unsigned long usbip_stub_debug_flags;


#ifndef USBIP_OS_NO_NR_ARGS
#define pr_fmt(lvl, fmt)	lvl ": " fmt "\n"
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define dbg_fmt(lvl, fmt)	pr_fmt(lvl, "%s:%d/%s: " fmt), __FILENAME__, __LINE__, __func__

#define dbg(fmt, args...) \
    if (usbip_use_debug) fprintf(stdout, dbg_fmt("<D>", fmt), ##args)

#define info(fmt, args...) \
    fprintf(stdout, dbg_fmt("<I>", fmt), ##args)

#define warn(fmt, args...) \
    fprintf(stderr, dbg_fmt("<W>", fmt), ##args)

#define err(fmt, args...) \
    fprintf(stderr, dbg_fmt("<E>", fmt), ##args)
#endif /* !USBIP_OS_NO_NR_ARGS */

#ifndef USBIP_OS_NO_NR_ARGS
#define dev_pfmt(dev, lvl, fmt) dbg_fmt(lvl, ": %d-%d: " fmt), libusb_get_bus_number(dev), libusb_get_device_address(dev)

#define dev_dbg(dev, fmt, args...) \
	fprintf(stdout, dev_pfmt(dev, "<D>", fmt), ##args)
#define dev_info(dev, fmt, args...) \
	fprintf(stdout, dev_pfmt(dev, "<I>", fmt), ##args)
#define dev_err(dev, fmt, args...) \
	fprintf(stdout, dev_pfmt(dev, "<E>", fmt), ##args)

enum {
    usbip_debug_xmit	= (1U << 0U),
    usbip_debug_eh		= (1U << 1U),
    usbip_debug_stub_rx	= (1U << 2U),
    usbip_debug_stub_tx	= (1U << 3U),
};

#define usbip_dbg_flag_xmit	(usbip_stub_debug_flags & (unsigned)usbip_debug_xmit)
#define usbip_dbg_flag_stub_rx	(usbip_stub_debug_flags & usbip_debug_stub_rx)
#define usbip_dbg_flag_stub_tx	(usbip_debug_flag & usbip_debug_stub_tx)


#define usbip_dbg_with_flag(flag, fmt, args...)		\
	do {						\
		if ((unsigned)flag & usbip_stub_debug_flags)		\
			dbg(fmt, ##args);		\
	} while (0)

#define usbip_dbg_xmit(fmt, args...) \
	usbip_dbg_with_flag(usbip_debug_xmit, fmt, ##args)
#define usbip_dbg_eh(fmt, args...) \
	usbip_dbg_with_flag(usbip_debug_eh, fmt, ##args)

#define usbip_dbg_stub_rx(fmt, args...) \
	usbip_dbg_with_flag(usbip_debug_stub_rx, fmt, ##args)
#define usbip_dbg_stub_tx(fmt, args...) \
	usbip_dbg_with_flag(usbip_debug_stub_tx, fmt, ##args)
#endif /*! USBIP_OS_NO_NR_ARGS */



void usbip_dump_buffer(char *buff, int bufflen);
//    struct libusb_device *dev = libusb_get_device(dev_handle);

#endif //__USBIP_DEBUG_H__