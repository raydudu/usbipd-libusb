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

#ifndef _USBIP_OS_H
#define _USBIP_OS_H

#if defined(__unix__)
#elif defined(__APPLE__)
#include "os_specifc/darwin/usbip_os.h"
#elif defined(_WIN32)
#include "os_specifc/win32/usbip_os.h"
#else
#error "unimplemented os"
#endif

#endif /* !_USBIP_OS_H */
