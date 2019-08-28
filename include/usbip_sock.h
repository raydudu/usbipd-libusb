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

#ifndef _USBIP_SOCK_H
#define _USBIP_SOCK_H

/*
 * This header is needed to clear duplicate definition errors in windows.
 */
#if defined(__unix__)
#elif defined(__APPLE__)
#elif defined(_WIN32)
#include "os_specific/win32/usbip_sock.h"
#else
#error "unimplemented os"
#endif

#endif /* !_USBIP_SOCK_H */
