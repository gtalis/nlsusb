// SPDX-License-Identifier: GPL-2.0+
/*
 * USB name database manipulation routines
 *
 * Copyright (C) 1999, 2000 Thomas Sailer (sailer@ife.ee.ethz.ch)
 * Copyright (C) 2018 Gilles Talis (gilles.talis@gmail.com)
 */

#ifndef _NAMES_H
#define _NAMES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <libusb.h>

/* ---------------------------------------------------------------------- */

const char *names_vendor(u_int16_t vendorid);
const char *names_product(u_int16_t vendorid, u_int16_t productid);
const char *names_class(u_int8_t classid);
const char *names_subclass(u_int8_t classid, u_int8_t subclassid);
const char *names_protocol(u_int8_t classid, u_int8_t subclassid,
			  u_int8_t protocolid);
const char *names_audioterminal(u_int16_t termt);
const char *names_videoterminal(u_int16_t termt);
const char *names_hid(u_int8_t hidd);
const char *names_reporttag(u_int8_t rt);
const char *names_huts(unsigned int data);
const char *names_hutus(unsigned int data);
const char *names_langid(u_int16_t langid);
const char *names_physdes(u_int8_t ph);
const char *names_bias(u_int8_t b);
const char *names_countrycode(unsigned int countrycode);

int get_vendor_string(char *buf, size_t size, u_int16_t vid);
int get_product_string(char *buf, size_t size, u_int16_t vid, u_int16_t pid);
int get_class_string(char *buf, size_t size, u_int8_t cls);
int get_subclass_string(char *buf, size_t size, u_int8_t cls, u_int8_t subcls);
int get_protocol_string(char *buf, size_t size, u_int8_t cls, u_int8_t subcls, u_int8_t proto);

int names_init(void);
void names_exit(void);

#ifdef __cplusplus
}
#endif

/* ---------------------------------------------------------------------- */
#endif /* _NAMES_H */
