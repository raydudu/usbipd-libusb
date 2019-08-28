/*
 *	usbip.h
 *
 *	USBIP uapi defines and function prototypes etc.
*/

#ifndef _UAPI_LINUX_USBIP_H
#define _UAPI_LINUX_USBIP_H

/* usbip device status - exported in usbip device sysfs status */
enum usbip_device_status {
	/* sdev is available. */
	SDEV_ST_AVAILABLE = 0x01,
	/* sdev is now used. */
	SDEV_ST_USED,
	/* sdev is unusable because of a fatal error. */
	SDEV_ST_ERROR,
};

#endif /* _UAPI_LINUX_USBIP_H */