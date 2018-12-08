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

#if 0
static void dump_ccid_device(const unsigned char *buf)
{
	unsigned int us;

	if (buf[0] < 54) {
		printf("      Warning: Descriptor too short\n");
		return;
	}
	printf("      ChipCard Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bcdCCID             %2x.%02x",
	       buf[0], buf[1], buf[3], buf[2]);
	if (buf[3] != 1 || buf[2] != 0)
		fputs("  (Warning: Only accurate for version 1.0)", stdout);
	putchar('\n');

	printf("        nMaxSlotIndex       %5u\n"
		"        bVoltageSupport     %5u  %s%s%s\n",
		buf[4],
		buf[5],
	       (buf[5] & 1) ? "5.0V " : "",
	       (buf[5] & 2) ? "3.0V " : "",
	       (buf[5] & 4) ? "1.8V " : "");

	us = convert_le_u32 (buf+6);
	printf("        dwProtocols         %5u ", us);
	if ((us & 1))
		fputs(" T=0", stdout);
	if ((us & 2))
		fputs(" T=1", stdout);
	if ((us & ~3))
		fputs(" (Invalid values detected)", stdout);
	putchar('\n');

	us = convert_le_u32(buf+10);
	printf("        dwDefaultClock      %5u\n", us);
	us = convert_le_u32(buf+14);
	printf("        dwMaxiumumClock     %5u\n", us);
	printf("        bNumClockSupported  %5u\n", buf[18]);
	us = convert_le_u32(buf+19);
	printf("        dwDataRate        %7u bps\n", us);
	us = convert_le_u32(buf+23);
	printf("        dwMaxDataRate     %7u bps\n", us);
	printf("        bNumDataRatesSupp.  %5u\n", buf[27]);

	us = convert_le_u32(buf+28);
	printf("        dwMaxIFSD           %5u\n", us);

	us = convert_le_u32(buf+32);
	printf("        dwSyncProtocols  %08X ", us);
	if ((us&1))
		fputs(" 2-wire", stdout);
	if ((us&2))
		fputs(" 3-wire", stdout);
	if ((us&4))
		fputs(" I2C", stdout);
	putchar('\n');

	us = convert_le_u32(buf+36);
	printf("        dwMechanical     %08X ", us);
	if ((us & 1))
		fputs(" accept", stdout);
	if ((us & 2))
		fputs(" eject", stdout);
	if ((us & 4))
		fputs(" capture", stdout);
	if ((us & 8))
		fputs(" lock", stdout);
	putchar('\n');

	us = convert_le_u32(buf+40);
	printf("        dwFeatures       %08X\n", us);
	if ((us & 0x0002))
		fputs("          Auto configuration based on ATR\n", stdout);
	if ((us & 0x0004))
		fputs("          Auto activation on insert\n", stdout);
	if ((us & 0x0008))
		fputs("          Auto voltage selection\n", stdout);
	if ((us & 0x0010))
		fputs("          Auto clock change\n", stdout);
	if ((us & 0x0020))
		fputs("          Auto baud rate change\n", stdout);
	if ((us & 0x0040))
		fputs("          Auto parameter negotiation made by CCID\n", stdout);
	else if ((us & 0x0080))
		fputs("          Auto PPS made by CCID\n", stdout);
	else if ((us & (0x0040 | 0x0080)))
		fputs("        WARNING: conflicting negotiation features\n", stdout);

	if ((us & 0x0100))
		fputs("          CCID can set ICC in clock stop mode\n", stdout);
	if ((us & 0x0200))
		fputs("          NAD value other than 0x00 accepted\n", stdout);
	if ((us & 0x0400))
		fputs("          Auto IFSD exchange\n", stdout);

	if ((us & 0x00010000))
		fputs("          TPDU level exchange\n", stdout);
	else if ((us & 0x00020000))
		fputs("          Short APDU level exchange\n", stdout);
	else if ((us & 0x00040000))
		fputs("          Short and extended APDU level exchange\n", stdout);
	else if ((us & 0x00070000))
		fputs("        WARNING: conflicting exchange levels\n", stdout);

	us = convert_le_u32(buf+44);
	printf("        dwMaxCCIDMsgLen     %5u\n", us);

	printf("        bClassGetResponse    ");
	if (buf[48] == 0xff)
		fputs("echo\n", stdout);
	else
		printf("  %02X\n", buf[48]);

	printf("        bClassEnvelope       ");
	if (buf[49] == 0xff)
		fputs("echo\n", stdout);
	else
		printf("  %02X\n", buf[48]);

	printf("        wlcdLayout           ");
	if (!buf[50] && !buf[51])
		fputs("none\n", stdout);
	else
		printf("%u cols %u lines\n", buf[50], buf[51]);

	printf("        bPINSupport         %5u ", buf[52]);
	if ((buf[52] & 1))
		fputs(" verification", stdout);
	if ((buf[52] & 2))
		fputs(" modification", stdout);
	putchar('\n');

	printf("        bMaxCCIDBusySlots   %5u\n", buf[53]);

	if (buf[0] > 54) {
		fputs("        junk             ", stdout);
		dump_bytes(buf+54, buf[0]-54);
	}
}

static void dump_dfu_interface(const unsigned char *buf)
{
	if (buf[1] != USB_DT_CS_DEVICE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 7)
		printf("      Warning: Descriptor too short\n");
	printf("      Device Firmware Upgrade Interface Descriptor:\n"
	       "        bLength                         %5u\n"
	       "        bDescriptorType                 %5u\n"
	       "        bmAttributes                    %5u\n",
	       buf[0], buf[1], buf[2]);
	if (buf[2] & 0xf0)
		printf("          (unknown attributes!)\n");
	printf("          Will %sDetach\n", (buf[2] & 0x08) ? "" : "Not ");
	printf("          Manifestation %s\n", (buf[2] & 0x04) ? "Tolerant" : "Intolerant");
	printf("          Upload %s\n", (buf[2] & 0x02) ? "Supported" : "Unsupported");
	printf("          Download %s\n", (buf[2] & 0x01) ? "Supported" : "Unsupported");
	printf("        wDetachTimeout                  %5u milliseconds\n"
	       "        wTransferSize                   %5u bytes\n",
	       buf[3] | (buf[4] << 8), buf[5] | (buf[6] << 8));

	/* DFU 1.0 defines no version code, DFU 1.1 does */
	if (buf[0] < 9)
		return;
	printf("        bcdDFUVersion                   %x.%02x\n",
			buf[8], buf[7]);
}
#endif

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
