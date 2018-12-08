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


void UsbDevice::dump_ccid_device(const unsigned char *buf, vector<string> &intf_info)
{
	unsigned int us;

	char line[128];
	char extra_info[32];

	if (buf[0] < 54) {
		snprintf(line, 128, "      Warning: Descriptor too short\n");
		intf_info.push_back(line);
		return;
	}
	snprintf(line, 128, "      ChipCard Interface Descriptor:\n");
	intf_info.push_back(line);
	snprintf(line, 128, "        bLength             %5u\n", buf[0]);
	intf_info.push_back(line);
	snprintf(line, 128, "        bDescriptorType     %5u\n", buf[1]);
	intf_info.push_back(line);
	snprintf(line, 128, "        bcdCCID             %2x.%02x", buf[3], buf[2]);

	if (buf[3] != 1 || buf[2] != 0) {
		strncat(line, "  (Warning: Only accurate for version 1.0)", 128);
	}
	intf_info.push_back(line);

	snprintf(line, 128, "        nMaxSlotIndex       %5u\n", buf[4]);
	intf_info.push_back(line);
	snprintf(line, 128, "        bVoltageSupport     %5u  %s%s%s\n",
		buf[5],
	       (buf[5] & 1) ? "5.0V " : "",
	       (buf[5] & 2) ? "3.0V " : "",
	       (buf[5] & 4) ? "1.8V " : "");
	intf_info.push_back(line);

	us = convert_le_u32 (buf+6);
	snprintf(line, 128, "        dwProtocols         %5u ", us);
	if ((us & 1))
		strncat(line, " T=0", 128);
	if ((us & 2))
		strncat(line, " T=1", 128);
	if ((us & ~3))
		strncat(line, " (Invalid values detected)", 128);
	intf_info.push_back(line);

	us = convert_le_u32(buf+10);
	snprintf(line, 128, "        dwDefaultClock      %5u\n", us);
	intf_info.push_back(line);
	us = convert_le_u32(buf+14);
	snprintf(line, 128, "        dwMaxiumumClock     %5u\n", us);
	intf_info.push_back(line);
	snprintf(line, 128, "        bNumClockSupported  %5u\n", buf[18]);
	intf_info.push_back(line);
	us = convert_le_u32(buf+19);
	snprintf(line, 128, "        dwDataRate        %7u bps\n", us);
	intf_info.push_back(line);
	us = convert_le_u32(buf+23);
	snprintf(line, 128, "        dwMaxDataRate     %7u bps\n", us);
	intf_info.push_back(line);
	snprintf(line, 128, "        bNumDataRatesSupp.  %5u\n", buf[27]);
	intf_info.push_back(line);

	us = convert_le_u32(buf+28);
	snprintf(line, 128, "        dwMaxIFSD           %5u\n", us);
	intf_info.push_back(line);

	us = convert_le_u32(buf+32);
	snprintf(line, 128, "        dwSyncProtocols  %08X ", us);
	if ((us&1))
		strncat(line, " 2-wire", 128);
	if ((us&2))
		strncat(line, " 3-wire", 128);
	if ((us&4))
		strncat(line, " I2C", 128);
	intf_info.push_back(line);

	us = convert_le_u32(buf+36);
	snprintf(line, 128, "        dwMechanical     %08X ", us);
	if ((us & 1))
		strncat(line, " accept", 128);
	if ((us & 2))
		strncat(line, " eject", 128);
	if ((us & 4))
		strncat(line, " capture", 128);
	if ((us & 8))
		strncat(line, " lock", 128);
	intf_info.push_back(line);

	us = convert_le_u32(buf+40);
	snprintf(line, 128, "        dwFeatures       %08X\n", us);
	intf_info.push_back(line);
	if ((us & 0x0002)) {
		snprintf(line, 128, "          Auto configuration based on ATR\n");
		intf_info.push_back(line);
	}
	if ((us & 0x0004)) {
		snprintf(line, 128, "          Auto activation on insert\n");
		intf_info.push_back(line);
	}
	if ((us & 0x0008)) {
		snprintf(line, 128, "          Auto voltage selection\n");
		intf_info.push_back(line);
	}
	if ((us & 0x0010)) {
		snprintf(line, 128, "          Auto clock change\n");
		intf_info.push_back(line);
	}
	if ((us & 0x0020)) {
		snprintf(line, 128, "          Auto baud rate change\n");
		intf_info.push_back(line);
	}
	if ((us & 0x0040)) {
		snprintf(line, 128, "          Auto parameter negotiation made by CCID\n");
		intf_info.push_back(line);
	}
	else if ((us & 0x0080)) {
		snprintf(line, 128, "          Auto PPS made by CCID\n");
		intf_info.push_back(line);
	}
	else if ((us & (0x0040 | 0x0080))) {
		snprintf(line, 128, "        WARNING: conflicting negotiation features\n");
		intf_info.push_back(line);
	}

	if ((us & 0x0100)) {
		snprintf(line, 128, "          CCID can set ICC in clock stop mode\n");
		intf_info.push_back(line);
	}
	if ((us & 0x0200)) {
		snprintf(line, 128, "          NAD value other than 0x00 accepted\n");
		intf_info.push_back(line);
	}
	if ((us & 0x0400)) {
		snprintf(line, 128, "          Auto IFSD exchange\n");
		intf_info.push_back(line);
	}

	if ((us & 0x00010000)) {
		snprintf(line, 128, "          TPDU level exchange\n");
		intf_info.push_back(line);
	}
	else if ((us & 0x00020000)) {
		snprintf(line, 128, "          Short APDU level exchange\n");
		intf_info.push_back(line);
	}
	else if ((us & 0x00040000)) {
		snprintf(line, 128, "          Short and extended APDU level exchange\n");
		intf_info.push_back(line);
	}
	else if ((us & 0x00070000)) {
		snprintf(line, 128, "        WARNING: conflicting exchange levels\n");
		intf_info.push_back(line);
	}

	us = convert_le_u32(buf+44);
	snprintf(line, 128, "        dwMaxCCIDMsgLen     %5u\n", us);
	intf_info.push_back(line);

	snprintf(line, 128, "        bClassGetResponse    ");
	if (buf[48] == 0xff) {
		strncat(line, "echo", 128);
	}
	else {
		snprintf(extra_info, 10, "  %02X\n", buf[48]);
		strncat(line, extra_info, 128);
	}

	strncat(line, "        bClassEnvelope       ", 128);
	if (buf[49] == 0xff) {
		strncat(line, "echo", 128);
	}
	else {
		snprintf(extra_info, 10, "  %02X\n", buf[48]);
		strncat(line, extra_info, 128);
	}

	strncat(line, "        wlcdLayout           ", 128);
	if (!buf[50] && !buf[51]) {
		strncat(line, "none", 128);
	}
	else {
		snprintf(extra_info, 32, "%u cols %u lines", buf[50], buf[51]);
		strncat(line, extra_info, 128);
	}
	intf_info.push_back(line);

	snprintf(line, 128, "        bPINSupport         %5u ", buf[52]);
	if ((buf[52] & 1))
		fputs(" verification", stdout);
	if ((buf[52] & 2))
		fputs(" modification", stdout);
	putchar('\n');

	snprintf(line, 128, "        bMaxCCIDBusySlots   %5u\n", buf[53]);
	intf_info.push_back(line);

	if (buf[0] > 54) {
		snprintf(line, 128, "        junk             ");
		dump_bytes(buf+54, buf[0]-54, (char **)&line, 128);
		intf_info.push_back(line);
	}
}

void UsbDevice::dump_dfu_interface(const unsigned char *buf, vector<string> &intf_info)
{
	char line[128];

	if (buf[1] != USB_DT_CS_DEVICE) {
		snprintf(line, 128, "      Warning: Invalid descriptor\n");
		intf_info.push_back(line);
	}
	else if (buf[0] < 7) {
		snprintf(line, 128, "      Warning: Descriptor too short\n");
		intf_info.push_back(line);
	}
	snprintf(line, 128, "      Device Firmware Upgrade Interface Descriptor:\n");
	intf_info.push_back(line);
	snprintf(line, 128, "        bLength                         %5u\n", buf[0]);
	intf_info.push_back(line);
	snprintf(line, 128, "        bDescriptorType                 %5u\n", buf[1]);
	intf_info.push_back(line);
	snprintf(line, 128, "        bmAttributes                    %5u\n", buf[2]);
	intf_info.push_back(line);

	if (buf[2] & 0xf0) {
		snprintf(line, 128, "          (unknown attributes!)\n");
		intf_info.push_back(line);
	}
	snprintf(line, 128, "          Will %sDetach\n", (buf[2] & 0x08) ? "" : "Not ");
	intf_info.push_back(line);
	snprintf(line, 128, "          Manifestation %s\n", (buf[2] & 0x04) ? "Tolerant" : "Intolerant");
	intf_info.push_back(line);
	snprintf(line, 128, "          Upload %s\n", (buf[2] & 0x02) ? "Supported" : "Unsupported");
	intf_info.push_back(line);
	snprintf(line, 128, "          Download %s\n", (buf[2] & 0x01) ? "Supported" : "Unsupported");
	intf_info.push_back(line);
	snprintf(line, 128, "        wDetachTimeout                  %5u milliseconds\n", buf[3] | (buf[4] << 8));
	intf_info.push_back(line);
	snprintf(line, 128, "        wTransferSize                   %5u bytes\n", buf[5] | (buf[6] << 8));
	intf_info.push_back(line);

	/* DFU 1.0 defines no version code, DFU 1.1 does */
	if (buf[0] < 9)
		return;
	snprintf(line, 128, "        bcdDFUVersion                   %x.%02x\n", buf[8], buf[7]);
	intf_info.push_back(line);
}

void UsbDevice::dump_altsetting(const struct libusb_interface_descriptor *interface, vector<string> &intf_info)
{
	char cls[128], subcls[128], proto[128];
	char *ifstr;

	const unsigned char *buf;
	unsigned size, i;

	get_class_string(cls, sizeof(cls), interface->bInterfaceClass);
	get_subclass_string(subcls, sizeof(subcls), interface->bInterfaceClass, interface->bInterfaceSubClass);
	get_protocol_string(proto, sizeof(proto), interface->bInterfaceClass, interface->bInterfaceSubClass, interface->bInterfaceProtocol);
	ifstr = get_dev_string(dev_handle_, interface->iInterface);

	char line[128];
	snprintf(line, 128, "    Interface Descriptor:\n");
	intf_info.push_back(line);
	snprintf(line, 128, "      bLength             %5u\n", interface->bLength);
	intf_info.push_back(line);
	snprintf(line, 128, "      bDescriptorType     %5u\n", interface->bDescriptorType);
	intf_info.push_back(line);
	snprintf(line, 128, "      bInterfaceNumber    %5u\n", interface->bInterfaceNumber);
	intf_info.push_back(line);
	snprintf(line, 128, "      bAlternateSetting   %5u\n", interface->bAlternateSetting);
	intf_info.push_back(line);
	snprintf(line, 128, "      bNumEndpoints       %5u\n", interface->bNumEndpoints);
	intf_info.push_back(line);
	snprintf(line, 128, "      bInterfaceClass     %5u %s\n", interface->bInterfaceClass, cls);
	intf_info.push_back(line);
	snprintf(line, 128, "      bInterfaceSubClass  %5u %s\n", interface->bInterfaceSubClass, subcls);
	intf_info.push_back(line);
	snprintf(line, 128, "      bInterfaceProtocol  %5u %s\n", interface->bInterfaceProtocol, proto);
	intf_info.push_back(line);
	snprintf(line, 128, "      iInterface          %5u %s\n", interface->iInterface, ifstr);
	intf_info.push_back(line);

	free(ifstr);

#if 0
	/* avoid re-ordering or hiding descriptors for display */
	if (interface->extra_length) {
		size = interface->extra_length;
		buf = interface->extra;
		while (size >= 2 * sizeof(u_int8_t)) {
			if (buf[0] < 2) {
				dump_junk(buf, "      ", size);
				break;
			}

			switch (buf[1]) {

			/* This is the polite way to provide class specific
			 * descriptors: explicitly tagged, using common class
			 * spec conventions.
			 */
			case USB_DT_CS_DEVICE:
			case USB_DT_CS_INTERFACE:
				switch (interface->bInterfaceClass) {
				case LIBUSB_CLASS_AUDIO:
					switch (interface->bInterfaceSubClass) {
					case 1:
						dump_audiocontrol_interface(dev, buf, interface->bInterfaceProtocol);
						break;
					case 2:
						dump_audiostreaming_interface(dev, buf, interface->bInterfaceProtocol);
						break;
					case 3:
						dump_midistreaming_interface(dev, buf);
						break;
					default:
						goto dump;
					}
					break;
				case LIBUSB_CLASS_COMM:
					dump_comm_descriptor(dev, buf,
						"      ");
					break;
				case USB_CLASS_VIDEO:
					switch (interface->bInterfaceSubClass) {
					case 1:
						dump_videocontrol_interface(dev, buf, interface->bInterfaceProtocol);
						break;
					case 2:
						dump_videostreaming_interface(buf);
						break;
					default:
						goto dump;
					}
					break;
				case USB_CLASS_APPLICATION:
					switch (interface->bInterfaceSubClass) {
					case 1:
						dump_dfu_interface(buf);
						break;
					default:
						goto dump;
					}
					break;
				case LIBUSB_CLASS_HID:
					dump_hid_device(dev, interface, buf);
					break;
				case USB_CLASS_CCID:
					dump_ccid_device(buf);
					break;
				default:
					goto dump;
				}
				break;

			/* This is the ugly way:  implicitly tagged,
			 * each class could redefine the type IDs.
			 */
			default:
				switch (interface->bInterfaceClass) {
				case LIBUSB_CLASS_HID:
					dump_hid_device(dev, interface, buf);
					break;
				case USB_CLASS_CCID:
					dump_ccid_device(buf);
					break;
				case 0xe0:	/* wireless */
					switch (interface->bInterfaceSubClass) {
					case 1:
						switch (interface->bInterfaceProtocol) {
						case 2:
							dump_rc_interface(buf);
							break;
						default:
							goto dump;
						}
						break;
					case 2:
						dump_wire_adapter(buf);
						break;
					default:
						goto dump;
					}
					break;
				case LIBUSB_CLASS_AUDIO:
					switch (buf[1]) {
					/* MISPLACED DESCRIPTOR */
					case USB_DT_CS_ENDPOINT:
						switch (interface->bInterfaceSubClass) {
						case 2:
							dump_audiostreaming_endpoint(dev, buf, interface->bInterfaceProtocol);
							break;
						default:
							goto dump;
						}
						break;
					default:
						goto dump;
					}
					break;
				default:
					/* ... not everything is class-specific */
					switch (buf[1]) {
					case USB_DT_OTG:
						/* handled separately */
						break;
					case USB_DT_INTERFACE_ASSOCIATION:
						dump_association(dev, buf);
						break;
					default:
dump:
						/* often a misplaced class descriptor */
						printf("      ** UNRECOGNIZED: ");
						dump_bytes(buf, buf[0]);
						break;
					}
				}
			}
			size -= buf[0];
			buf += buf[0];
		}
	}

	for (i = 0 ; i < interface->bNumEndpoints ; i++)
		dump_endpoint(dev, interface, &interface->endpoint[i]);

#endif
}

void UsbDevice::dump_interface(const struct libusb_interface *interface, vector<string> &intf_info)
{
	for (int i = 0; i < interface->num_altsetting; i++)
		dump_altsetting(&interface->altsetting[i], intf_info);
}
