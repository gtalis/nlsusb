// SPDX-License-Identifier: GPL-2.0+
/*
 * Misc USB routines
 *
 * Copyright (C) 2003 Aurelien Jarno (aurelien@aurel32.net)
 * Copyright (C) 2018 Gilles Talis
 */

#ifndef _USBMISC_H
#define _USBMISC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libusb.h>

/* ---------------------------------------------------------------------- */

extern libusb_device *get_usb_device(libusb_context *ctx, const char *path);

char *get_dev_string(libusb_device_handle *dev, u_int8_t id);

#ifdef __cplusplus
}
#endif

/* ---------------------------------------------------------------------- */
#endif /* _USBMISC_H */
