/*
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
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

/*
 * USB/IP API to create command(s) and daemons with arbitrary network protocol.
 */

#ifndef __UAPI_LINUX_USBIP_API_H
#define __UAPI_LINUX_USBIP_API_H

#ifdef __cplusplus
extern "C"
{
#endif

void usbip_set_use_debug(int val);
void usbip_set_use_stderr(int val);
void usbip_set_use_syslog(int val);

#ifdef USBIP_WITH_LIBUSB
void usbip_set_debug_flags(unsigned long flags);
#endif

int usbipd_recv_pdu(int sock_fd,
		    const char *host, const char *port);

#ifdef __cplusplus
}
#endif

#endif /* !__UAPI_LINUX_USBIP_API_H */
