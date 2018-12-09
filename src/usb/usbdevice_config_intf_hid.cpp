/*
    Copyright (C) 1999-2001, 2003 Thomas Sailer (t.sailer@alumni.ethz.ch)
    Copyright (C) 2003-2005 David Brownell
    Copyright (C) 2018  Gilles Talis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "usbdevice.h"
#include "names.h"
#include "usbmisc.h"

#include <stdio.h>
#include <string.h>

using namespace std;

// Not sure why this is needed in lsusb.c
static int do_report_desc = 1;

/* ---------------------------------------------------------------------- */

/*
 * HID descriptor
 */
static void dump_unit(unsigned int data, unsigned int len, vector<string> &intf_info)
{
	string systems[] = { "None", "SI Linear", "SI Rotation",
			"English Linear", "English Rotation" };

	string units[][8] = {
		{ "None", "None", "None", "None", "None",
				"None", "None", "None" },
		{ "None", "Centimeter", "Gram", "Seconds", "Kelvin",
				"Ampere", "Candela", "None" },
		{ "None", "Radians",    "Gram", "Seconds", "Kelvin",
				"Ampere", "Candela", "None" },
		{ "None", "Inch",       "Slug", "Seconds", "Fahrenheit",
				"Ampere", "Candela", "None" },
		{ "None", "Degrees",    "Slug", "Seconds", "Fahrenheit",
				"Ampere", "Candela", "None" },
	};
	char indent[] = "                            ";

	unsigned int i;
	unsigned int sys;
	int earlier_unit = 0;
	
	char line[128];
	char extra_info[64];

	snprintf(line, 128, "%s", indent);

	/* First nibble tells us which system we're in. */
	sys = data & 0xf;
	data >>= 4;

	if (sys > 4) {
		if (sys == 0xf)
			strncat(line, "System: Vendor defined, Unit: (unknown)", 50);
		else
			strncat(line, "System: Reserved, Unit: (unknown)", 50);
		intf_info.push_back(line);
		return;
	} else {
		snprintf(extra_info, 64, "System: %s, Unit: ", systems[sys].c_str() );
		strncat(line, extra_info, 64);
	}
	for (i = 1 ; i < len * 2 ; i++) {
		char nibble = data & 0xf;
		data >>= 4;
		if (nibble != 0) {
			if (earlier_unit++ > 0)
				strncat(line, "*", 2);
			snprintf(extra_info, 15, "%s", units[sys][i].c_str() );
			strncat(line, extra_info, 15);
			if (nibble != 1) {
				/* This is a _signed_ nibble(!) */

				int val = nibble & 0x7;
				if (nibble & 0x08)
					val = -((0x7 & ~val) + 1);
				snprintf(extra_info, 8, "^%d", val);
				strncat(line, extra_info, 8);
			}
		}
	}
	if (earlier_unit == 0)
		strncat(line, "(None)", 7);
	intf_info.push_back(line);
}



static void dump_report_desc(unsigned char *b, int l, vector<string> &intf_info)
{
	unsigned int j, bsize, btag, btype, data = 0xffff, hut = 0xffff;
	int i;
	string types[] = { "Main", "Global", "Local", "reserved" };
	char indent[] = "                            ";

	char line[128];
	char extra_info[32];

	snprintf(line, 128, "          Report Descriptor: (length is %d)\n", l);
	intf_info.push_back(line);

	for (i = 0; i < l; ) {
		bsize = b[i] & 0x03;
		if (bsize == 3)
			bsize = 4;
		btype = b[i] & (0x03 << 2);
		btag = b[i] & ~0x03; /* 2 LSB bits encode length */
		snprintf(line, 128, "            Item(%-6s): %s, data=", types[btype>>2].c_str(),
				names_reporttag(btag));
		if (bsize > 0) {
			strncat(line, " [ ", 5);
			data = 0;
			for (j = 0; j < bsize; j++) {
				snprintf(extra_info, 32, "0x%02x ", b[i+1+j]);
				strncat(line, extra_info, 32);
				data += (b[i+1+j] << (8*j));
			}
			snprintf(extra_info, 15,  "] %d", data);
			strncat(line, extra_info, 15);
		} else {
			strncat(line, "none", 5);
		}
		intf_info.push_back(line);

		switch (btag) {
		case 0x04: /* Usage Page */
			snprintf(line, 128, "%s%s\n", indent, names_huts(data));
			intf_info.push_back(line);
			hut = data;
			break;

		case 0x08: /* Usage */
		case 0x18: /* Usage Minimum */
		case 0x28: /* Usage Maximum */
			snprintf(line, 128, "%s%s\n", indent,
			       names_hutus((hut << 16) + data));
			intf_info.push_back(line);
			break;

		case 0x54: /* Unit Exponent */
			snprintf(line, 128, "%sUnit Exponent: %i\n", indent,
			       (signed char)data);
			intf_info.push_back(line);
			break;

		case 0x64: /* Unit */
			//snprintf(line, 128, "%s", indent);
			dump_unit(data, bsize, intf_info);
			break;

		case 0xa0: /* Collection */
			snprintf(line, 128, "%s", indent);
			switch (data) {
			case 0x00:
				strncat(line, "Physical", 20);
				break;

			case 0x01:
				strncat(line, "Application", 20);
				break;

			case 0x02:
				strncat(line, "Logical", 20);
				break;

			case 0x03:
				strncat(line, "Report", 20);
				break;

			case 0x04:
				strncat(line, "Named Array", 20);
				break;

			case 0x05:
				strncat(line, "Usage Switch", 20);
				break;

			case 0x06:
				strncat(line, "Usage Modifier", 20);
				break;

			default:
				if (data & 0x80)
					strncat(line, "Vendor defined", 32);
				else
					strncat(line, "Reserved for future use", 32);
			}
			intf_info.push_back(line);
			break;
		case 0x80: /* Input */
		case 0x90: /* Output */
		case 0xb0: /* Feature */
			snprintf(line, 128, "%s%s %s %s %s %s\n%s%s %s %s %s\n",
			       indent,
			       data & 0x01 ? "Constant" : "Data",
			       data & 0x02 ? "Variable" : "Array",
			       data & 0x04 ? "Relative" : "Absolute",
			       data & 0x08 ? "Wrap" : "No_Wrap",
			       data & 0x10 ? "Non_Linear" : "Linear",
			       indent,
			       data & 0x20 ? "No_Preferred_State" : "Preferred_State",
			       data & 0x40 ? "Null_State" : "No_Null_Position",
			       data & 0x80 ? "Volatile" : "Non_Volatile",
			       data & 0x100 ? "Buffered Bytes" : "Bitfield");
			intf_info.push_back(line);
			break;
		}
		i += 1 + bsize;
	}
}

void UsbDevice::dump_hid_device(
			    const struct libusb_interface_descriptor *interface,
			    const unsigned char *buf,
			    vector<string> &intf_info)
{
	unsigned int i, len;
	unsigned int n;
	unsigned char dbuf[8192];

	char line[128];

	if (buf[1] != LIBUSB_DT_HID) {
		snprintf(line, 128, "      Warning: Invalid descriptor\n");
		intf_info.push_back(line);
	}
	else if (buf[0] < 6+3*buf[5]) {
		snprintf(line, 128, "      Warning: Descriptor too short\n");
		intf_info.push_back(line);
	}

	snprintf(line, 128, "        HID Device Descriptor:\n");
	intf_info.push_back(line);
	snprintf(line, 128, "          bLength             %5u\n", buf[0]);
	intf_info.push_back(line);
	snprintf(line, 128, "          bDescriptorType     %5u\n", buf[1]);
	intf_info.push_back(line);
	snprintf(line, 128, "          bcdHID              %2x.%02x\n", buf[3], buf[2]);
	intf_info.push_back(line);
	snprintf(line, 128, "          bCountryCode        %5u %s\n", buf[4], names_countrycode(buf[4]) ? : "Unknown");
	intf_info.push_back(line);
	snprintf(line, 128, "          bNumDescriptors     %5u\n", buf[5]);
	intf_info.push_back(line);

	for (i = 0; i < buf[5]; i++) {
		snprintf(line, 128, "          bDescriptorType     %5u %s\n", buf[6+3*i], names_hid(buf[6+3*i]));
		intf_info.push_back(line);
		snprintf(line, 128, "          wDescriptorLength   %5u\n", buf[7+3*i] | (buf[8+3*i] << 8));
		intf_info.push_back(line);
	}

	dump_junk(buf, "        ", 6+3*buf[5], (char **)line, 128);
	intf_info.push_back(line);
	if (!do_report_desc)
		return;

	if (!dev_handle_) {
		snprintf(line, 128, "         Report Descriptors: \n");
		intf_info.push_back(line);
		snprintf(line, 128, "           ** UNAVAILABLE **\n");
		intf_info.push_back(line);
		return;
	}

	for (i = 0; i < buf[5]; i++) {
		/* we are just interested in report descriptors*/
		if (buf[6+3*i] != LIBUSB_DT_REPORT)
			continue;
		len = buf[7+3*i] | (buf[8+3*i] << 8);
		if (len > (unsigned int)sizeof(dbuf)) {
			snprintf(line, 128, "report descriptor too long\n");
			intf_info.push_back(line);
			continue;
		}
		if (libusb_claim_interface(dev_handle_, interface->bInterfaceNumber) == 0) {
			int retries = 4;
			n = 0;
			while (n < len && retries--)
				n = usb_control_msg(dev_handle_,
					 LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
						| LIBUSB_RECIPIENT_INTERFACE,
					 LIBUSB_REQUEST_GET_DESCRIPTOR,
					 (LIBUSB_DT_REPORT << 8),
					 interface->bInterfaceNumber,
					 dbuf, len,
					 CTRL_TIMEOUT);

			if (n > 0) {
				if (n < len) {
					snprintf(line, 128, "          Warning: incomplete report descriptor\n");
					intf_info.push_back(line);
				}
				dump_report_desc(dbuf, n, intf_info);
			}
			libusb_release_interface(dev_handle_, interface->bInterfaceNumber);
		} else {
			/* recent Linuxes require claim() for RECIP_INTERFACE,
			 * so "rmmod hid" will often make these available.
			 */
			snprintf(line, 128, "         Report Descriptors: \n");
			intf_info.push_back(line);
			snprintf(line, 128, "           ** UNAVAILABLE **\n");
			intf_info.push_back(line);
		}
	}
}