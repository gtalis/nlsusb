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

/* ---------------------------------------------------------------------- */

/*
 * HID descriptor
 */

static void dump_report_desc(unsigned char *b, int l)
{
	unsigned int j, bsize, btag, btype, data = 0xffff, hut = 0xffff;
	int i;
	char *types[4] = { "Main", "Global", "Local", "reserved" };
	char indent[] = "                            ";

	printf("          Report Descriptor: (length is %d)\n", l);
	for (i = 0; i < l; ) {
		bsize = b[i] & 0x03;
		if (bsize == 3)
			bsize = 4;
		btype = b[i] & (0x03 << 2);
		btag = b[i] & ~0x03; /* 2 LSB bits encode length */
		printf("            Item(%-6s): %s, data=", types[btype>>2],
				names_reporttag(btag));
		if (bsize > 0) {
			printf(" [ ");
			data = 0;
			for (j = 0; j < bsize; j++) {
				printf("0x%02x ", b[i+1+j]);
				data += (b[i+1+j] << (8*j));
			}
			printf("] %d", data);
		} else
			printf("none");
		printf("\n");
		switch (btag) {
		case 0x04: /* Usage Page */
			printf("%s%s\n", indent, names_huts(data));
			hut = data;
			break;

		case 0x08: /* Usage */
		case 0x18: /* Usage Minimum */
		case 0x28: /* Usage Maximum */
			printf("%s%s\n", indent,
			       names_hutus((hut << 16) + data));
			break;

		case 0x54: /* Unit Exponent */
			printf("%sUnit Exponent: %i\n", indent,
			       (signed char)data);
			break;

		case 0x64: /* Unit */
			printf("%s", indent);
			dump_unit(data, bsize);
			break;

		case 0xa0: /* Collection */
			printf("%s", indent);
			switch (data) {
			case 0x00:
				printf("Physical\n");
				break;

			case 0x01:
				printf("Application\n");
				break;

			case 0x02:
				printf("Logical\n");
				break;

			case 0x03:
				printf("Report\n");
				break;

			case 0x04:
				printf("Named Array\n");
				break;

			case 0x05:
				printf("Usage Switch\n");
				break;

			case 0x06:
				printf("Usage Modifier\n");
				break;

			default:
				if (data & 0x80)
					printf("Vendor defined\n");
				else
					printf("Reserved for future use.\n");
			}
			break;
		case 0x80: /* Input */
		case 0x90: /* Output */
		case 0xb0: /* Feature */
			printf("%s%s %s %s %s %s\n%s%s %s %s %s\n",
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
			break;
		}
		i += 1 + bsize;
	}
}

static void dump_hid_device(libusb_device_handle *dev,
			    const struct libusb_interface_descriptor *interface,
			    const unsigned char *buf)
{
	unsigned int i, len;
	unsigned int n;
	unsigned char dbuf[8192];

	if (buf[1] != LIBUSB_DT_HID)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 6+3*buf[5])
		printf("      Warning: Descriptor too short\n");
	printf("        HID Device Descriptor:\n"
	       "          bLength             %5u\n"
	       "          bDescriptorType     %5u\n"
	       "          bcdHID              %2x.%02x\n"
	       "          bCountryCode        %5u %s\n"
	       "          bNumDescriptors     %5u\n",
	       buf[0], buf[1], buf[3], buf[2], buf[4],
	       names_countrycode(buf[4]) ? : "Unknown", buf[5]);
	for (i = 0; i < buf[5]; i++)
		printf("          bDescriptorType     %5u %s\n"
		       "          wDescriptorLength   %5u\n",
		       buf[6+3*i], names_hid(buf[6+3*i]),
		       buf[7+3*i] | (buf[8+3*i] << 8));
	dump_junk(buf, "        ", 6+3*buf[5]);
	if (!do_report_desc)
		return;

	if (!dev) {
		printf("         Report Descriptors: \n"
		       "           ** UNAVAILABLE **\n");
		return;
	}

	for (i = 0; i < buf[5]; i++) {
		/* we are just interested in report descriptors*/
		if (buf[6+3*i] != LIBUSB_DT_REPORT)
			continue;
		len = buf[7+3*i] | (buf[8+3*i] << 8);
		if (len > (unsigned int)sizeof(dbuf)) {
			printf("report descriptor too long\n");
			continue;
		}
		if (libusb_claim_interface(dev, interface->bInterfaceNumber) == 0) {
			int retries = 4;
			n = 0;
			while (n < len && retries--)
				n = usb_control_msg(dev,
					 LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
						| LIBUSB_RECIPIENT_INTERFACE,
					 LIBUSB_REQUEST_GET_DESCRIPTOR,
					 (LIBUSB_DT_REPORT << 8),
					 interface->bInterfaceNumber,
					 dbuf, len,
					 CTRL_TIMEOUT);

			if (n > 0) {
				if (n < len)
					printf("          Warning: incomplete report descriptor\n");
				dump_report_desc(dbuf, n);
			}
			libusb_release_interface(dev, interface->bInterfaceNumber);
		} else {
			/* recent Linuxes require claim() for RECIP_INTERFACE,
			 * so "rmmod hid" will often make these available.
			 */
			printf("         Report Descriptors: \n"
			       "           ** UNAVAILABLE **\n");
		}
	}
}

static char *
dump_comm_descriptor(libusb_device_handle *dev, const unsigned char *buf, char *indent)
{
	int		tmp;
	char		*str = NULL;
	char		*type;

	switch (buf[2]) {
	case 0:
		type = "Header";
		if (buf[0] != 5)
			goto bad;
		printf("%sCDC Header:\n"
		       "%s  bcdCDC               %x.%02x\n",
		       indent,
		       indent, buf[4], buf[3]);
		break;
	case 0x01:		/* call management functional desc */
		type = "Call Management";
		if (buf[0] != 5)
			goto bad;
		printf("%sCDC Call Management:\n"
		       "%s  bmCapabilities       0x%02x\n",
		       indent,
		       indent, buf[3]);
		if (buf[3] & 0x01)
			printf("%s    call management\n", indent);
		if (buf[3] & 0x02)
			printf("%s    use DataInterface\n", indent);
		printf("%s  bDataInterface          %d\n", indent, buf[4]);
		break;
	case 0x02:		/* acm functional desc */
		type = "ACM";
		if (buf[0] != 4)
			goto bad;
		printf("%sCDC ACM:\n"
		       "%s  bmCapabilities       0x%02x\n",
		       indent,
		       indent, buf[3]);
		if (buf[3] & 0x08)
			printf("%s    connection notifications\n", indent);
		if (buf[3] & 0x04)
			printf("%s    sends break\n", indent);
		if (buf[3] & 0x02)
			printf("%s    line coding and serial state\n", indent);
		if (buf[3] & 0x01)
			printf("%s    get/set/clear comm features\n", indent);
		break;
#if 0
	case 0x03:		/* direct line management */
	case 0x04:		/* telephone ringer */
	case 0x05:		/* telephone call and line state reporting */
#endif
	case 0x06:		/* union desc */
		type = "Union";
		if (buf[0] < 5)
			goto bad;
		printf("%sCDC Union:\n"
		       "%s  bMasterInterface        %d\n"
		       "%s  bSlaveInterface         ",
		       indent,
		       indent, buf[3],
		       indent);
		for (tmp = 4; tmp < buf[0]; tmp++)
			printf("%d ", buf[tmp]);
		printf("\n");
		break;
	case 0x07:		/* country selection functional desc */
		type = "Country Selection";
		if (buf[0] < 6 || (buf[0] & 1) != 0)
			goto bad;
		str = get_dev_string(dev, buf[3]);
		printf("%sCountry Selection:\n"
		       "%s  iCountryCodeRelDate     %4d %s\n",
		       indent,
		       indent, buf[3], (buf[3] && *str) ? str : "(?\?)");
		for (tmp = 4; tmp < buf[0]; tmp += 2) {
			printf("%s  wCountryCode          0x%02x%02x\n",
				indent, buf[tmp], buf[tmp + 1]);
		}
		break;
	case 0x08:		/* telephone operational modes */
		type = "Telephone Operations";
		if (buf[0] != 4)
			goto bad;
		printf("%sCDC Telephone operations:\n"
		       "%s  bmCapabilities       0x%02x\n",
		       indent,
		       indent, buf[3]);
		if (buf[3] & 0x04)
			printf("%s    computer centric mode\n", indent);
		if (buf[3] & 0x02)
			printf("%s    standalone mode\n", indent);
		if (buf[3] & 0x01)
			printf("%s    simple mode\n", indent);
		break;
#if 0
	case 0x09:		/* USB terminal */
#endif
	case 0x0a:		/* network channel terminal */
		type = "Network Channel Terminal";
		if (buf[0] != 7)
			goto bad;
		str = get_dev_string(dev, buf[4]);
		printf("%sNetwork Channel Terminal:\n"
		       "%s  bEntityId               %3d\n"
		       "%s  iName                   %3d %s\n"
		       "%s  bChannelIndex           %3d\n"
		       "%s  bPhysicalInterface      %3d\n",
		       indent,
		       indent, buf[3],
		       indent, buf[4], str,
		       indent, buf[5],
		       indent, buf[6]);
		break;
#if 0
	case 0x0b:		/* protocol unit */
	case 0x0c:		/* extension unit */
	case 0x0d:		/* multi-channel management */
	case 0x0e:		/* CAPI control management*/
#endif
	case 0x0f:		/* ethernet functional desc */
		type = "Ethernet";
		if (buf[0] != 13)
			goto bad;
		str = get_dev_string(dev, buf[3]);
		tmp = buf[7] << 8;
		tmp |= buf[6]; tmp <<= 8;
		tmp |= buf[5]; tmp <<= 8;
		tmp |= buf[4];
		printf("%sCDC Ethernet:\n"
		       "%s  iMacAddress             %10d %s\n"
		       "%s  bmEthernetStatistics    0x%08x\n",
		       indent,
		       indent, buf[3], (buf[3] && *str) ? str : "(?\?)",
		       indent, tmp);
		/* FIXME dissect ALL 28 bits */
		printf("%s  wMaxSegmentSize         %10d\n"
		       "%s  wNumberMCFilters            0x%04x\n"
		       "%s  bNumberPowerFilters     %10d\n",
		       indent, (buf[9]<<8)|buf[8],
		       indent, (buf[11]<<8)|buf[10],
		       indent, buf[12]);
		break;
#if 0
	case 0x10:		/* ATM networking */
#endif
	case 0x11:		/* WHCM functional desc */
		type = "WHCM version";
		if (buf[0] != 5)
			goto bad;
		printf("%sCDC WHCM:\n"
		       "%s  bcdVersion           %x.%02x\n",
		       indent,
		       indent, buf[4], buf[3]);
		break;
	case 0x12:		/* MDLM functional desc */
		type = "MDLM";
		if (buf[0] != 21)
			goto bad;
		printf("%sCDC MDLM:\n"
		       "%s  bcdCDC               %x.%02x\n"
		       "%s  bGUID               %s\n",
		       indent,
		       indent, buf[4], buf[3],
		       indent, get_guid(buf + 5));
		break;
	case 0x13:		/* MDLM detail desc */
		type = "MDLM detail";
		if (buf[0] < 5)
			goto bad;
		printf("%sCDC MDLM detail:\n"
		       "%s  bGuidDescriptorType  %02x\n"
		       "%s  bDetailData         ",
		       indent,
		       indent, buf[3],
		       indent);
		dump_bytes(buf + 4, buf[0] - 4);
		break;
	case 0x14:		/* device management functional desc */
		type = "Device Management";
		if (buf[0] != 7)
			goto bad;
		printf("%sCDC Device Management:\n"
		       "%s  bcdVersion           %x.%02x\n"
		       "%s  wMaxCommand          %d\n",
		       indent,
		       indent, buf[4], buf[3],
		       indent, (buf[6] << 8) | buf[5]);
		break;
	case 0x15:		/* OBEX functional desc */
		type = "OBEX";
		if (buf[0] != 5)
			goto bad;
		printf("%sCDC OBEX:\n"
		       "%s  bcdVersion           %x.%02x\n",
		       indent,
		       indent, buf[4], buf[3]);
		break;
	case 0x16:		/* command set functional desc */
		type = "Command Set";
		if (buf[0] != 22)
			goto bad;
		str = get_dev_string(dev, buf[5]);
		printf("%sCDC Command Set:\n"
		       "%s  bcdVersion           %x.%02x\n"
		       "%s  iCommandSet          %4d %s\n"
		       "%s  bGUID                %s\n",
		       indent,
		       indent, buf[4], buf[3],
		       indent, buf[5], (buf[5] && *str) ? str : "(?\?)",
		       indent, get_guid(buf + 6));
		break;
#if 0
	case 0x17:		/* command set detail desc */
	case 0x18:		/* telephone control model functional desc */
#endif
	case 0x1a:		/* NCM functional desc */
		type = "NCM";
		if (buf[0] != 6)
			goto bad;
		printf("%sCDC NCM:\n"
		       "%s  bcdNcmVersion        %x.%02x\n"
		       "%s  bmNetworkCapabilities 0x%02x\n",
		       indent,
		       indent, buf[4], buf[3],
		       indent, buf[5]);
		if (buf[5] & 1<<5)
			printf("%s    8-byte ntb input size\n", indent);
		if (buf[5] & 1<<4)
			printf("%s    crc mode\n", indent);
		if (buf[5] & 1<<3)
			printf("%s    max datagram size\n", indent);
		if (buf[5] & 1<<2)
			printf("%s    encapsulated commands\n", indent);
		if (buf[5] & 1<<1)
			printf("%s    net address\n", indent);
		if (buf[5] & 1<<0)
			printf("%s    packet filter\n", indent);
		break;
	case 0x1b:		/* MBIM functional desc */
		type = "MBIM";
		if (buf[0] != 12)
			goto bad;
		printf("%sCDC MBIM:\n"
		       "%s  bcdMBIMVersion       %x.%02x\n"
		       "%s  wMaxControlMessage   %d\n"
		       "%s  bNumberFilters       %d\n"
		       "%s  bMaxFilterSize       %d\n"
		       "%s  wMaxSegmentSize      %d\n"
		       "%s  bmNetworkCapabilities 0x%02x\n",
		       indent,
		       indent, buf[4], buf[3],
		       indent, (buf[6] << 8) | buf[5],
		       indent, buf[7],
		       indent, buf[8],
		       indent, (buf[10] << 8) | buf[9],
		       indent, buf[11]);
		if (buf[11] & 0x20)
			printf("%s    8-byte ntb input size\n", indent);
		if (buf[11] & 0x08)
			printf("%s    max datagram size\n", indent);
		break;
	case 0x1c:		/* MBIM extended functional desc */
		type = "MBIM Extended";
		if (buf[0] != 8)
			goto bad;
		printf("%sCDC MBIM Extended:\n"
		       "%s  bcdMBIMExtendedVersion          %2x.%02x\n"
		       "%s  bMaxOutstandingCommandMessages    %3d\n"
		       "%s  wMTU                            %5d\n",
		       indent,
		       indent, buf[4], buf[3],
		       indent, buf[5],
		       indent, buf[6] | (buf[7] << 8));
		break;
	default:
		/* FIXME there are about a dozen more descriptor types */
		printf("%sUNRECOGNIZED CDC: ", indent);
		dump_bytes(buf, buf[0]);
		return "unrecognized comm descriptor";
	}

	free(str);

	return 0;

bad:
	printf("%sINVALID CDC (%s): ", indent, type);
	dump_bytes(buf, buf[0]);
	return "corrupt comm descriptor";
}



/* ---------------------------------------------------------------------- */

/*
 * Audio Class descriptor dump
 */

static void dump_audio_subtype(libusb_device_handle *dev,
                               const char *name,
                               const struct desc * const desc[3],
                               const unsigned char *buf,
                               int protocol,
                               unsigned int indent)
{
	static const char * const strings[] = { "UAC1", "UAC2", "UAC3" };
	unsigned int idx = 0;

	switch (protocol) {
	case USB_AUDIO_CLASS_2: idx = 1; break;
	case USB_AUDIO_CLASS_3: idx = 2; break;
	}

	printf("(%s)\n", name);

	if (desc[idx] == NULL) {
		printf("%*sWarning: %s descriptors are illegal for %s\n",
		       indent * 2, "", name, strings[idx]);
		return;
	}

	/* Skip the first three bytes; those common fields have already
	 * been dumped. */
	desc_dump(dev, desc[idx], buf + 3, buf[0] - 3, indent);
}

/* USB Audio Class subtypes */
enum uac_interface_subtype {
	UAC_INTERFACE_SUBTYPE_AC_DESCRIPTOR_UNDEFINED = 0x00,
	UAC_INTERFACE_SUBTYPE_HEADER                  = 0x01,
	UAC_INTERFACE_SUBTYPE_INPUT_TERMINAL          = 0x02,
	UAC_INTERFACE_SUBTYPE_OUTPUT_TERMINAL         = 0x03,
	UAC_INTERFACE_SUBTYPE_EXTENDED_TERMINAL       = 0x04,
	UAC_INTERFACE_SUBTYPE_MIXER_UNIT              = 0x05,
	UAC_INTERFACE_SUBTYPE_SELECTOR_UNIT           = 0x06,
	UAC_INTERFACE_SUBTYPE_FEATURE_UNIT            = 0x07,
	UAC_INTERFACE_SUBTYPE_EFFECT_UNIT             = 0x08,
	UAC_INTERFACE_SUBTYPE_PROCESSING_UNIT         = 0x09,
	UAC_INTERFACE_SUBTYPE_EXTENSION_UNIT          = 0x0a,
	UAC_INTERFACE_SUBTYPE_CLOCK_SOURCE            = 0x0b,
	UAC_INTERFACE_SUBTYPE_CLOCK_SELECTOR          = 0x0c,
	UAC_INTERFACE_SUBTYPE_CLOCK_MULTIPLIER        = 0x0d,
	UAC_INTERFACE_SUBTYPE_SAMPLE_RATE_CONVERTER   = 0x0e,
	UAC_INTERFACE_SUBTYPE_CONNECTORS              = 0x0f,
	UAC_INTERFACE_SUBTYPE_POWER_DOMAIN            = 0x10,
};

/*
 * UAC1, UAC2, and UAC3 define bDescriptorSubtype differently for the
 * AudioControl interface, so we need to do some ugly remapping:
 *
 * val  | UAC1            | UAC2                  | UAC3
 * -----|-----------------|-----------------------|---------------------
 * 0x00 | AC UNDEFINED    | AC UNDEFINED          | AC UNDEFINED
 * 0x01 | HEADER          | HEADER                | HEADER
 * 0x02 | INPUT_TERMINAL  | INPUT_TERMINAL        | INPUT_TERMINAL
 * 0x03 | OUTPUT_TERMINAL | OUTPUT_TERMINAL       | OUTPUT_TERMINAL
 * 0x04 | MIXER_UNIT      | MIXER_UNIT            | EXTENDED_TERMINAL
 * 0x05 | SELECTOR_UNIT   | SELECTOR_UNIT         | MIXER_UNIT
 * 0x06 | FEATURE_UNIT    | FEATURE_UNIT          | SELECTOR_UNIT
 * 0x07 | PROCESSING_UNIT | EFFECT_UNIT           | FEATURE_UNIT
 * 0x08 | EXTENSION_UNIT  | PROCESSING_UNIT       | EFFECT_UNIT
 * 0x09 | -               | EXTENSION_UNIT        | PROCESSING_UNIT
 * 0x0a | -               | CLOCK_SOURCE          | EXTENSION_UNIT
 * 0x0b | -               | CLOCK_SELECTOR        | CLOCK_SOURCE
 * 0x0c | -               | CLOCK_MULTIPLIER      | CLOCK_SELECTOR
 * 0x0d | -               | SAMPLE_RATE_CONVERTER | CLOCK_MULTIPLIER
 * 0x0e | -               | -                     | SAMPLE_RATE_CONVERTER
 * 0x0f | -               | -                     | CONNECTORS
 * 0x10 | -               | -                     | POWER_DOMAIN
 */
static enum uac_interface_subtype get_uac_interface_subtype(unsigned char c, int protocol)
{
	switch (protocol) {
	case USB_AUDIO_CLASS_1:
		switch(c) {
		case 0x04: return UAC_INTERFACE_SUBTYPE_MIXER_UNIT;
		case 0x05: return UAC_INTERFACE_SUBTYPE_SELECTOR_UNIT;
		case 0x06: return UAC_INTERFACE_SUBTYPE_FEATURE_UNIT;
		case 0x07: return UAC_INTERFACE_SUBTYPE_PROCESSING_UNIT;
		case 0x08: return UAC_INTERFACE_SUBTYPE_EXTENSION_UNIT;
		}
		break;
	case USB_AUDIO_CLASS_2:
		switch(c) {
		case 0x04: return UAC_INTERFACE_SUBTYPE_MIXER_UNIT;
		case 0x05: return UAC_INTERFACE_SUBTYPE_SELECTOR_UNIT;
		case 0x06: return UAC_INTERFACE_SUBTYPE_FEATURE_UNIT;
		case 0x07: return UAC_INTERFACE_SUBTYPE_EFFECT_UNIT;
		case 0x08: return UAC_INTERFACE_SUBTYPE_PROCESSING_UNIT;
		case 0x09: return UAC_INTERFACE_SUBTYPE_EXTENSION_UNIT;
		case 0x0a: return UAC_INTERFACE_SUBTYPE_CLOCK_SOURCE;
		case 0x0b: return UAC_INTERFACE_SUBTYPE_CLOCK_SELECTOR;
		case 0x0c: return UAC_INTERFACE_SUBTYPE_CLOCK_MULTIPLIER;
		case 0x0d: return UAC_INTERFACE_SUBTYPE_SAMPLE_RATE_CONVERTER;
		}
		break;
	case USB_AUDIO_CLASS_3:
		/* No mapping required. */
		break;
	default:
		/* Unknown protocol */
		break;
	}

	/* If the protocol was unknown, or the value was not known to require
	 * mapping, just return it unchanged. */
	return c;
}

static void dump_audiocontrol_interface(libusb_device_handle *dev, const unsigned char *buf, int protocol)
{
	enum uac_interface_subtype subtype;

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      AudioControl Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);

	subtype = get_uac_interface_subtype(buf[2], protocol);

	switch (subtype) {
	case UAC_INTERFACE_SUBTYPE_HEADER:
		dump_audio_subtype(dev, "HEADER", desc_audio_ac_header, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_INPUT_TERMINAL:
		dump_audio_subtype(dev, "INPUT_TERMINAL", desc_audio_ac_input_terminal, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_OUTPUT_TERMINAL:
		dump_audio_subtype(dev, "OUTPUT_TERMINAL", desc_audio_ac_output_terminal, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_MIXER_UNIT:
		dump_audio_subtype(dev, "MIXER_UNIT", desc_audio_ac_mixer_unit, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_SELECTOR_UNIT:
		dump_audio_subtype(dev, "SELECTOR_UNIT", desc_audio_ac_selector_unit, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_FEATURE_UNIT:
		dump_audio_subtype(dev, "FEATURE_UNIT", desc_audio_ac_feature_unit, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_PROCESSING_UNIT:
		dump_audio_subtype(dev, "PROCESSING_UNIT", desc_audio_ac_processing_unit, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_EXTENSION_UNIT:
		dump_audio_subtype(dev, "EXTENSION_UNIT", desc_audio_ac_extension_unit, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_CLOCK_SOURCE:
		dump_audio_subtype(dev, "CLOCK_SOURCE", desc_audio_ac_clock_source, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_CLOCK_SELECTOR:
		dump_audio_subtype(dev, "CLOCK_SELECTOR", desc_audio_ac_clock_selector, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_CLOCK_MULTIPLIER:
		dump_audio_subtype(dev, "CLOCK_MULTIPLIER", desc_audio_ac_clock_multiplier, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_SAMPLE_RATE_CONVERTER:
		dump_audio_subtype(dev, "SAMPLING_RATE_CONVERTER", desc_audio_ac_clock_multiplier, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_EFFECT_UNIT:
		dump_audio_subtype(dev, "EFFECT_UNIT", desc_audio_ac_effect_unit, buf, protocol, 4);
		break;

	case UAC_INTERFACE_SUBTYPE_POWER_DOMAIN:
		dump_audio_subtype(dev, "POWER_DOMAIN", desc_audio_ac_power_domain, buf, protocol, 4);
		break;

	default:
		printf("(unknown)\n"
		       "        Invalid desc subtype:");
		dump_bytes(buf+3, buf[0]-3);
		break;
	}
}


static void dump_audiostreaming_interface(libusb_device_handle *dev, const unsigned char *buf, int protocol)
{
	static const char * const fmtItag[] = {
		"TYPE_I_UNDEFINED", "PCM", "PCM8", "IEEE_FLOAT", "ALAW", "MULAW" };
	static const char * const fmtIItag[] = { "TYPE_II_UNDEFINED", "MPEG", "AC-3" };
	static const char * const fmtIIItag[] = {
		"TYPE_III_UNDEFINED", "IEC1937_AC-3", "IEC1937_MPEG-1_Layer1",
		"IEC1937_MPEG-Layer2/3/NOEXT", "IEC1937_MPEG-2_EXT",
		"IEC1937_MPEG-2_Layer1_LS", "IEC1937_MPEG-2_Layer2/3_LS" };
	unsigned int i, j, fmttag;
	const char *fmtptr = "undefined";
	char *name = NULL;

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      AudioStreaming Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);
	switch (buf[2]) {
	case 0x01: /* AS_GENERAL */
		dump_audio_subtype(dev, "AS_GENERAL", desc_audio_as_interface, buf, protocol, 4);
		break;

	case 0x02: /* FORMAT_TYPE */
		printf("(FORMAT_TYPE)\n");
		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			if (buf[0] < 8)
				printf("      Warning: Descriptor too short\n");
			printf("        bFormatType         %5u ", buf[3]);
			switch (buf[3]) {
			case 0x01: /* FORMAT_TYPE_I */
				printf("(FORMAT_TYPE_I)\n");
				j = buf[7] ? (buf[7]*3+8) : 14;
				if (buf[0] < j)
					printf("      Warning: Descriptor too short\n");
				printf("        bNrChannels         %5u\n"
				       "        bSubframeSize       %5u\n"
				       "        bBitResolution      %5u\n"
				       "        bSamFreqType        %5u %s\n",
				       buf[4], buf[5], buf[6], buf[7], buf[7] ? "Discrete" : "Continuous");
				if (!buf[7])
					printf("        tLowerSamFreq     %7u\n"
					       "        tUpperSamFreq     %7u\n",
					       buf[8] | (buf[9] << 8) | (buf[10] << 16), buf[11] | (buf[12] << 8) | (buf[13] << 16));
				else
					for (i = 0; i < buf[7]; i++)
						printf("        tSamFreq[%2u]      %7u\n", i,
						       buf[8+3*i] | (buf[9+3*i] << 8) | (buf[10+3*i] << 16));
				dump_junk(buf, "        ", j);
				break;

			case 0x02: /* FORMAT_TYPE_II */
				printf("(FORMAT_TYPE_II)\n");
				j = buf[8] ? (buf[7]*3+9) : 15;
				if (buf[0] < j)
					printf("      Warning: Descriptor too short\n");
				printf("        wMaxBitRate         %5u\n"
				       "        wSamplesPerFrame    %5u\n"
				       "        bSamFreqType        %5u %s\n",
				       buf[4] | (buf[5] << 8), buf[6] | (buf[7] << 8), buf[8], buf[8] ? "Discrete" : "Continuous");
				if (!buf[8])
					printf("        tLowerSamFreq     %7u\n"
					       "        tUpperSamFreq     %7u\n",
					       buf[9] | (buf[10] << 8) | (buf[11] << 16), buf[12] | (buf[13] << 8) | (buf[14] << 16));
				else
					for (i = 0; i < buf[8]; i++)
						printf("        tSamFreq[%2u]      %7u\n", i,
						       buf[9+3*i] | (buf[10+3*i] << 8) | (buf[11+3*i] << 16));
				dump_junk(buf, "        ", j);
				break;

			case 0x03: /* FORMAT_TYPE_III */
				printf("(FORMAT_TYPE_III)\n");
				j = buf[7] ? (buf[7]*3+8) : 14;
				if (buf[0] < j)
					printf("      Warning: Descriptor too short\n");
				printf("        bNrChannels         %5u\n"
				       "        bSubframeSize       %5u\n"
				       "        bBitResolution      %5u\n"
				       "        bSamFreqType        %5u %s\n",
				       buf[4], buf[5], buf[6], buf[7], buf[7] ? "Discrete" : "Continuous");
				if (!buf[7])
					printf("        tLowerSamFreq     %7u\n"
					       "        tUpperSamFreq     %7u\n",
					       buf[8] | (buf[9] << 8) | (buf[10] << 16), buf[11] | (buf[12] << 8) | (buf[13] << 16));
				else
					for (i = 0; i < buf[7]; i++)
						printf("        tSamFreq[%2u]      %7u\n", i,
						       buf[8+3*i] | (buf[9+3*i] << 8) | (buf[10+3*i] << 16));
				dump_junk(buf, "        ", j);
				break;

			default:
				printf("(unknown)\n"
				       "        Invalid desc format type:");
				dump_bytes(buf+4, buf[0]-4);
			}

			break;

		case USB_AUDIO_CLASS_2:
			printf("        bFormatType         %5u ", buf[3]);
			switch (buf[3]) {
			case 0x01: /* FORMAT_TYPE_I */
				printf("(FORMAT_TYPE_I)\n");
				if (buf[0] < 6)
					printf("      Warning: Descriptor too short\n");
				printf("        bSubslotSize        %5u\n"
				       "        bBitResolution      %5u\n",
				       buf[4], buf[5]);
				dump_junk(buf, "        ", 6);
				break;

			case 0x02: /* FORMAT_TYPE_II */
				printf("(FORMAT_TYPE_II)\n");
				if (buf[0] < 8)
					printf("      Warning: Descriptor too short\n");
				printf("        wMaxBitRate         %5u\n"
				       "        wSlotsPerFrame      %5u\n",
				       buf[4] | (buf[5] << 8),
				       buf[6] | (buf[7] << 8));
				dump_junk(buf, "        ", 8);
				break;

			case 0x03: /* FORMAT_TYPE_III */
				printf("(FORMAT_TYPE_III)\n");
				if (buf[0] < 6)
					printf("      Warning: Descriptor too short\n");
				printf("        bSubslotSize        %5u\n"
				       "        bBitResolution      %5u\n",
				       buf[4], buf[5]);
				dump_junk(buf, "        ", 6);
				break;

			case 0x04: /* FORMAT_TYPE_IV */
				printf("(FORMAT_TYPE_IV)\n");
				if (buf[0] < 4)
					printf("      Warning: Descriptor too short\n");
				printf("        bFormatType         %5u\n", buf[3]);
				dump_junk(buf, "        ", 4);
				break;

			default:
				printf("(unknown)\n"
				       "        Invalid desc format type:");
				dump_bytes(buf+4, buf[0]-4);
			}

			break;
		} /* switch (protocol) */

		break;

	case 0x03: /* FORMAT_SPECIFIC */
		printf("(FORMAT_SPECIFIC)\n");
		if (buf[0] < 5)
			printf("      Warning: Descriptor too short\n");
		fmttag = buf[3] | (buf[4] << 8);
		if (fmttag <= 5)
			fmtptr = fmtItag[fmttag];
		else if (fmttag >= 0x1000 && fmttag <= 0x1002)
			fmtptr = fmtIItag[fmttag & 0xfff];
		else if (fmttag >= 0x2000 && fmttag <= 0x2006)
			fmtptr = fmtIIItag[fmttag & 0xfff];
		printf("        wFormatTag          %5u %s\n", fmttag, fmtptr);
		switch (fmttag) {
		case 0x1001: /* MPEG */
			if (buf[0] < 8)
				printf("      Warning: Descriptor too short\n");
			printf("        bmMPEGCapabilities 0x%04x\n",
			       buf[5] | (buf[6] << 8));
			if (buf[5] & 0x01)
				printf("          Layer I\n");
			if (buf[5] & 0x02)
				printf("          Layer II\n");
			if (buf[5] & 0x04)
				printf("          Layer III\n");
			if (buf[5] & 0x08)
				printf("          MPEG-1 only\n");
			if (buf[5] & 0x10)
				printf("          MPEG-1 dual-channel\n");
			if (buf[5] & 0x20)
				printf("          MPEG-2 second stereo\n");
			if (buf[5] & 0x40)
				printf("          MPEG-2 7.1 channel augmentation\n");
			if (buf[5] & 0x80)
				printf("          Adaptive multi-channel prediction\n");
			printf("          MPEG-2 multilingual support: ");
			switch (buf[6] & 3) {
			case 0:
				printf("Not supported\n");
				break;

			case 1:
				printf("Supported at Fs\n");
				break;

			case 2:
				printf("Reserved\n");
				break;

			default:
				printf("Supported at Fs and 1/2Fs\n");
				break;
			}
			printf("        bmMPEGFeatures       0x%02x\n", buf[7]);
			printf("          Internal Dynamic Range Control: ");
			switch ((buf[7] >> 4) & 3) {
			case 0:
				printf("not supported\n");
				break;

			case 1:
				printf("supported but not scalable\n");
				break;

			case 2:
				printf("scalable, common boost and cut scaling value\n");
				break;

			default:
				printf("scalable, separate boost and cut scaling value\n");
				break;
			}
			dump_junk(buf, "        ", 8);
			break;

		case 0x1002: /* AC-3 */
			if (buf[0] < 10)
				printf("      Warning: Descriptor too short\n");
			printf("        bmBSID         0x%08x\n"
			       "        bmAC3Features        0x%02x\n",
			       buf[5] | (buf[6] << 8) | (buf[7] << 16) | (buf[8] << 24), buf[9]);
			if (buf[9] & 0x01)
				printf("          RF mode\n");
			if (buf[9] & 0x02)
				printf("          Line mode\n");
			if (buf[9] & 0x04)
				printf("          Custom0 mode\n");
			if (buf[9] & 0x08)
				printf("          Custom1 mode\n");
			printf("          Internal Dynamic Range Control: ");
			switch ((buf[9] >> 4) & 3) {
			case 0:
				printf("not supported\n");
				break;

			case 1:
				printf("supported but not scalable\n");
				break;

			case 2:
				printf("scalable, common boost and cut scaling value\n");
				break;

			default:
				printf("scalable, separate boost and cut scaling value\n");
				break;
			}
			dump_junk(buf, "        ", 8);
			break;

		default:
			printf("(unknown)\n"
			       "        Invalid desc format type:");
			dump_bytes(buf+4, buf[0]-4);
		}
		break;

	default:
		printf("        Invalid desc subtype:");
		dump_bytes(buf+3, buf[0]-3);
		break;
	}

	free(name);
}

static void dump_audiostreaming_endpoint(libusb_device_handle *dev, const unsigned char *buf, int protocol)
{
	static const char * const subtype[] = { "invalid", "EP_GENERAL" };

	if (buf[1] != USB_DT_CS_ENDPOINT)
		printf("      Warning: Invalid descriptor\n");

	printf("        AudioStreaming Endpoint Descriptor:\n"
	       "          bLength             %5u\n"
	       "          bDescriptorType     %5u\n"
	       "          bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);

	dump_audio_subtype(dev, subtype[buf[2] == 1],
			desc_audio_as_isochronous_audio_data_endpoint, buf, protocol, 5);
}

static void dump_midistreaming_interface(libusb_device_handle *dev, const unsigned char *buf)
{
	static const char * const jacktypes[] = {"Undefined", "Embedded", "External"};
	char *jackstr = NULL;
	unsigned int j, tlength, capssize;
	unsigned long caps;

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      MIDIStreaming Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);
	switch (buf[2]) {
	case 0x01:
		printf("(HEADER)\n");
		if (buf[0] < 7)
			printf("      Warning: Descriptor too short\n");
		tlength = buf[5] | (buf[6] << 8);
		printf("        bcdADC              %2x.%02x\n"
		       "        wTotalLength       0x%04x\n",
		       buf[4], buf[3], tlength);
		dump_junk(buf, "        ", 7);
		break;

	case 0x02:
		printf("(MIDI_IN_JACK)\n");
		if (buf[0] < 6)
			printf("      Warning: Descriptor too short\n");
		jackstr = get_dev_string(dev, buf[5]);
		printf("        bJackType           %5u %s\n"
		       "        bJackID             %5u\n"
		       "        iJack               %5u %s\n",
		       buf[3], buf[3] < 3 ? jacktypes[buf[3]] : "Invalid",
		       buf[4], buf[5], jackstr);
		dump_junk(buf, "        ", 6);
		break;

	case 0x03:
		printf("(MIDI_OUT_JACK)\n");
		if (buf[0] < 9)
			printf("      Warning: Descriptor too short\n");
		printf("        bJackType           %5u %s\n"
		       "        bJackID             %5u\n"
		       "        bNrInputPins        %5u\n",
		       buf[3], buf[3] < 3 ? jacktypes[buf[3]] : "Invalid",
		       buf[4], buf[5]);
		for (j = 0; j < buf[5]; j++) {
			printf("        baSourceID(%2u)      %5u\n"
			       "        BaSourcePin(%2u)     %5u\n",
			       j, buf[2*j+6], j, buf[2*j+7]);
		}
		j = 6+buf[5]*2; /* midi10.pdf says, incorrectly: 5+2*p */
		jackstr = get_dev_string(dev, buf[j]);
		printf("        iJack               %5u %s\n",
		       buf[j], jackstr);
		dump_junk(buf, "        ", j+1);
		break;

	case 0x04:
		printf("(ELEMENT)\n");
		if (buf[0] < 12)
			printf("      Warning: Descriptor too short\n");
		printf("        bElementID          %5u\n"
		       "        bNrInputPins        %5u\n",
		       buf[3], buf[4]);
		for (j = 0; j < buf[4]; j++) {
			printf("        baSourceID(%2u)      %5u\n"
			       "        BaSourcePin(%2u)     %5u\n",
			       j, buf[2*j+5], j, buf[2*j+6]);
		}
		j = 5+buf[4]*2;
		printf("        bNrOutputPins       %5u\n"
		       "        bInTerminalLink     %5u\n"
		       "        bOutTerminalLink    %5u\n"
		       "        bElCapsSize         %5u\n",
		       buf[j], buf[j+1], buf[j+2], buf[j+3]);
		capssize = buf[j+3];
		caps = 0;
		for (j = 0; j < capssize; j++)
			caps |= (buf[j+9+buf[4]*2] << (8*j));
		printf("        bmElementCaps  0x%08lx\n", caps);
		if (caps & 0x01)
			printf("          Undefined\n");
		if (caps & 0x02)
			printf("          MIDI Clock\n");
		if (caps & 0x04)
			printf("          MTC (MIDI Time Code)\n");
		if (caps & 0x08)
			printf("          MMC (MIDI Machine Control)\n");
		if (caps & 0x10)
			printf("          GM1 (General MIDI v.1)\n");
		if (caps & 0x20)
			printf("          GM2 (General MIDI v.2)\n");
		if (caps & 0x40)
			printf("          GS MIDI Extension\n");
		if (caps & 0x80)
			printf("          XG MIDI Extension\n");
		if (caps & 0x100)
			printf("          EFX\n");
		if (caps & 0x200)
			printf("          MIDI Patch Bay\n");
		if (caps & 0x400)
			printf("          DLS1 (Downloadable Sounds Level 1)\n");
		if (caps & 0x800)
			printf("          DLS2 (Downloadable Sounds Level 2)\n");
		j = 9+2*buf[4]+capssize;
		jackstr = get_dev_string(dev, buf[j]);
		printf("        iElement            %5u %s\n", buf[j], jackstr);
		dump_junk(buf, "        ", j+1);
		break;

	default:
		printf("\n        Invalid desc subtype: ");
		dump_bytes(buf+3, buf[0]-3);
		break;
	}

	free(jackstr);
}

static void dump_midistreaming_endpoint(const unsigned char *buf)
{
	unsigned int j;

	if (buf[1] != USB_DT_CS_ENDPOINT)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 5)
		printf("      Warning: Descriptor too short\n");
	printf("        MIDIStreaming Endpoint Descriptor:\n"
	       "          bLength             %5u\n"
	       "          bDescriptorType     %5u\n"
	       "          bDescriptorSubtype  %5u (%s)\n"
	       "          bNumEmbMIDIJack     %5u\n",
	       buf[0], buf[1], buf[2], buf[2] == 1 ? "GENERAL" : "Invalid", buf[3]);
	for (j = 0; j < buf[3]; j++)
		printf("          baAssocJackID(%2u)   %5u\n", j, buf[4+j]);
	dump_junk(buf, "          ", 4+buf[3]);
}

/*
 * Video Class descriptor dump
 */

static void dump_videocontrol_interface(libusb_device_handle *dev, const unsigned char *buf, int protocol)
{
	static const char * const ctrlnames[] = {
		"Brightness", "Contrast", "Hue", "Saturation", "Sharpness", "Gamma",
		"White Balance Temperature", "White Balance Component", "Backlight Compensation",
		"Gain", "Power Line Frequency", "Hue, Auto", "White Balance Temperature, Auto",
		"White Balance Component, Auto", "Digital Multiplier", "Digital Multiplier Limit",
		"Analog Video Standard", "Analog Video Lock Status", "Contrast, Auto"
	};
	static const char * const camctrlnames[] = {
		"Scanning Mode", "Auto-Exposure Mode", "Auto-Exposure Priority",
		"Exposure Time (Absolute)", "Exposure Time (Relative)", "Focus (Absolute)",
		"Focus (Relative)", "Iris (Absolute)", "Iris (Relative)", "Zoom (Absolute)",
		"Zoom (Relative)", "PanTilt (Absolute)", "PanTilt (Relative)",
		"Roll (Absolute)", "Roll (Relative)", "Reserved", "Reserved", "Focus, Auto",
		"Privacy", "Focus, Simple", "Window", "Region of Interest"
	};
	static const char * const enctrlnames[] = {
		"Select Layer", "Profile and Toolset", "Video Resolution", "Minimum Frame Interval",
		"Slice Mode", "Rate Control Mode", "Average Bit Rate", "CPB Size", "Peak Bit Rate",
		"Quantization Parameter", "Synchronization and Long-Term Reference Frame",
		"Long-Term Buffer", "Picture Long-Term Reference", "LTR Validation",
		"Level IDC", "SEI Message", "QP Range", "Priority ID", "Start or Stop Layer/View",
		"Error Resiliency"
	};
	static const char * const stdnames[] = {
		"None", "NTSC - 525/60", "PAL - 625/50", "SECAM - 625/50",
		"NTSC - 625/50", "PAL - 525/60" };
	unsigned int i, ctrls, stds, n, p, termt, freq;
	char *term = NULL, termts[128];

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      VideoControl Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);
	switch (buf[2]) {
	case 0x01:  /* HEADER */
		printf("(HEADER)\n");
		n = buf[11];
		if (buf[0] < 12+n)
			printf("      Warning: Descriptor too short\n");
		freq = buf[7] | (buf[8] << 8) | (buf[9] << 16) | (buf[10] << 24);
		printf("        bcdUVC              %2x.%02x\n"
		       "        wTotalLength       0x%04x\n"
		       "        dwClockFrequency    %5u.%06uMHz\n"
		       "        bInCollection       %5u\n",
		       buf[4], buf[3], buf[5] | (buf[6] << 8), freq / 1000000,
		       freq % 1000000, n);
		for (i = 0; i < n; i++)
			printf("        baInterfaceNr(%2u)   %5u\n", i, buf[12+i]);
		dump_junk(buf, "        ", 12+n);
		break;

	case 0x02:  /* INPUT_TERMINAL */
		printf("(INPUT_TERMINAL)\n");
		term = get_dev_string(dev, buf[7]);
		termt = buf[4] | (buf[5] << 8);
		n = termt == 0x0201 ? 7 : 0;
		get_videoterminal_string(termts, sizeof(termts), termt);
		if (buf[0] < 8 + n)
			printf("      Warning: Descriptor too short\n");
		printf("        bTerminalID         %5u\n"
		       "        wTerminalType      0x%04x %s\n"
		       "        bAssocTerminal      %5u\n",
		       buf[3], termt, termts, buf[6]);
		printf("        iTerminal           %5u %s\n",
		       buf[7], term);
		if (termt == 0x0201) {
			n += buf[14];
			printf("        wObjectiveFocalLengthMin  %5u\n"
			       "        wObjectiveFocalLengthMax  %5u\n"
			       "        wOcularFocalLength        %5u\n"
			       "        bControlSize              %5u\n",
			       buf[8] | (buf[9] << 8), buf[10] | (buf[11] << 8),
			       buf[12] | (buf[13] << 8), buf[14]);
			ctrls = 0;
			for (i = 0; i < 3 && i < buf[14]; i++)
				ctrls = (ctrls << 8) | buf[8+n-i-1];
			printf("        bmControls           0x%08x\n", ctrls);
			if (protocol == USB_VIDEO_PROTOCOL_15) {
				for (i = 0; i < 22; i++)
					if ((ctrls >> i) & 1)
						printf("          %s\n", camctrlnames[i]);
			}
			else {
				for (i = 0; i < 19; i++)
					if ((ctrls >> i) & 1)
						printf("          %s\n", camctrlnames[i]);
			}
		}
		dump_junk(buf, "        ", 8+n);
		break;

	case 0x03:  /* OUTPUT_TERMINAL */
		printf("(OUTPUT_TERMINAL)\n");
		term = get_dev_string(dev, buf[8]);
		termt = buf[4] | (buf[5] << 8);
		get_videoterminal_string(termts, sizeof(termts), termt);
		if (buf[0] < 9)
			printf("      Warning: Descriptor too short\n");
		printf("        bTerminalID         %5u\n"
		       "        wTerminalType      0x%04x %s\n"
		       "        bAssocTerminal      %5u\n"
		       "        bSourceID           %5u\n"
		       "        iTerminal           %5u %s\n",
		       buf[3], termt, termts, buf[6], buf[7], buf[8], term);
		dump_junk(buf, "        ", 9);
		break;

	case 0x04:  /* SELECTOR_UNIT */
		printf("(SELECTOR_UNIT)\n");
		p = buf[4];
		if (buf[0] < 6+p)
			printf("      Warning: Descriptor too short\n");
		term = get_dev_string(dev, buf[5+p]);

		printf("        bUnitID             %5u\n"
		       "        bNrInPins           %5u\n",
		       buf[3], p);
		for (i = 0; i < p; i++)
			printf("        baSource(%2u)        %5u\n", i, buf[5+i]);
		printf("        iSelector           %5u %s\n",
		       buf[5+p], term);
		dump_junk(buf, "        ", 6+p);
		break;

	case 0x05:  /* PROCESSING_UNIT */
		printf("(PROCESSING_UNIT)\n");
		n = buf[7];
		term = get_dev_string(dev, buf[8+n]);
		if (buf[0] < 10+n)
			printf("      Warning: Descriptor too short\n");
		printf("        bUnitID             %5u\n"
		       "        bSourceID           %5u\n"
		       "        wMaxMultiplier      %5u\n"
		       "        bControlSize        %5u\n",
		       buf[3], buf[4], buf[5] | (buf[6] << 8), n);
		ctrls = 0;
		for (i = 0; i < 3 && i < n; i++)
			ctrls = (ctrls << 8) | buf[8+n-i-1];
		printf("        bmControls     0x%08x\n", ctrls);
		if (protocol == USB_VIDEO_PROTOCOL_15) {
			for (i = 0; i < 19; i++)
				if ((ctrls >> i) & 1)
					printf("          %s\n", ctrlnames[i]);
		}
		else {
			for (i = 0; i < 18; i++)
				if ((ctrls >> i) & 1)
					printf("          %s\n", ctrlnames[i]);
		}
		stds = buf[9+n];
		printf("        iProcessing         %5u %s\n"
		       "        bmVideoStandards     0x%02x\n", buf[8+n], term, stds);
		for (i = 0; i < 6; i++)
			if ((stds >> i) & 1)
				printf("          %s\n", stdnames[i]);
		break;

	case 0x06:  /* EXTENSION_UNIT */
		printf("(EXTENSION_UNIT)\n");
		p = buf[21];
		n = buf[22+p];
		term = get_dev_string(dev, buf[23+p+n]);
		if (buf[0] < 24+p+n)
			printf("      Warning: Descriptor too short\n");
		printf("        bUnitID             %5u\n"
		       "        guidExtensionCode         %s\n"
		       "        bNumControl         %5u\n"
		       "        bNrPins             %5u\n",
		       buf[3], get_guid(&buf[4]), buf[20], buf[21]);
		for (i = 0; i < p; i++)
			printf("        baSourceID(%2u)      %5u\n", i, buf[22+i]);
		printf("        bControlSize        %5u\n", buf[22+p]);
		for (i = 0; i < n; i++)
			printf("        bmControls(%2u)       0x%02x\n", i, buf[23+p+i]);
		printf("        iExtension          %5u %s\n",
		       buf[23+p+n], term);
		dump_junk(buf, "        ", 24+p+n);
		break;

	case 0x07: /* ENCODING UNIT */
		printf("(ENCODING UNIT)\n");
		term = get_dev_string(dev, buf[5]);
		if (buf[0] < 13)
			printf("      Warning: Descriptor too short\n");
		printf("        bUnitID             %5u\n"
		       "        bSourceID           %5u\n"
		       "        iEncoding           %5u %s\n"
		       "        bControlSize        %5u\n",
		       buf[3], buf[4], buf[5], term, buf[6]);
		ctrls = 0;
		for (i = 0; i < 3; i++)
			ctrls = (ctrls << 8) | buf[9-i];
		printf("        bmControls              0x%08x\n", ctrls);
		for (i = 0; i < 20;  i++)
			if ((ctrls >> i) & 1)
				printf("          %s\n", enctrlnames[i]);
		for (i = 0; i< 3; i++)
			ctrls = (ctrls << 8) | buf[12-i];
		printf("        bmControlsRuntime       0x%08x\n", ctrls);
		for (i = 0; i < 20; i++)
			if ((ctrls >> i) & 1)
				printf("          %s\n", enctrlnames[i]);
		break;

	default:
		printf("(unknown)\n"
		       "        Invalid desc subtype:");
		dump_bytes(buf+3, buf[0]-3);
		break;
	}

	free(term);
}

static void dump_videostreaming_interface(const unsigned char *buf)
{
	static const char * const colorPrims[] = { "Unspecified", "BT.709,sRGB",
		"BT.470-2 (M)", "BT.470-2 (B,G)", "SMPTE 170M", "SMPTE 240M" };
	static const char * const transferChars[] = { "Unspecified", "BT.709",
		"BT.470-2 (M)", "BT.470-2 (B,G)", "SMPTE 170M", "SMPTE 240M",
		"Linear", "sRGB"};
	static const char * const matrixCoeffs[] = { "Unspecified", "BT.709",
		"FCC", "BT.470-2 (B,G)", "SMPTE 170M (BT.601)", "SMPTE 240M" };
	unsigned int i, m, n, p, flags, len;

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      VideoStreaming Interface Descriptor:\n"
	       "        bLength                         %5u\n"
	       "        bDescriptorType                 %5u\n"
	       "        bDescriptorSubtype              %5u ",
	       buf[0], buf[1], buf[2]);
	switch (buf[2]) {
	case 0x01: /* INPUT_HEADER */
		printf("(INPUT_HEADER)\n");
		p = buf[3];
		n = buf[12];
		if (buf[0] < 13+p*n)
			printf("      Warning: Descriptor too short\n");
		printf("        bNumFormats                     %5u\n"
		       "        wTotalLength                   0x%04x\n"
		       "        bEndPointAddress                %5u\n"
		       "        bmInfo                          %5u\n"
		       "        bTerminalLink                   %5u\n"
		       "        bStillCaptureMethod             %5u\n"
		       "        bTriggerSupport                 %5u\n"
		       "        bTriggerUsage                   %5u\n"
		       "        bControlSize                    %5u\n",
		       p, buf[4] | (buf[5] << 8), buf[6], buf[7], buf[8],
		       buf[9], buf[10], buf[11], n);
		for (i = 0; i < p; i++)
			printf(
			"        bmaControls(%2u)                 %5u\n",
				i, buf[13+i*n]);
		dump_junk(buf, "        ", 13+p*n);
		break;

	case 0x02: /* OUTPUT_HEADER */
		printf("(OUTPUT_HEADER)\n");
		p = buf[3];
		n = buf[8];
		if (buf[0] < 9+p*n)
			printf("      Warning: Descriptor too short\n");
		printf("        bNumFormats                 %5u\n"
		       "        wTotalLength               0x%04x\n"
		       "        bEndpointAddress            %5u\n"
		       "        bTerminalLink               %5u\n"
		       "        bControlSize                %5u\n",
		       p, buf[4] | (buf[5] << 8), buf[6], buf[7], n);
		for (i = 0; i < p; i++)
			printf(
			"        bmaControls(%2u)             %5u\n",
				i, buf[9+i*n]);
		dump_junk(buf, "        ", 9+p*n);
		break;

	case 0x03: /* STILL_IMAGE_FRAME */
		printf("(STILL_IMAGE_FRAME)\n");
		n = buf[4];
		m = buf[5+4*n];
		if (buf[0] < 6+4*n+m)
			printf("      Warning: Descriptor too short\n");
		printf("        bEndpointAddress                %5u\n"
		       "        bNumImageSizePatterns             %3u\n",
		       buf[3], n);
		for (i = 0; i < n; i++)
			printf("        wWidth(%2u)                      %5u\n"
			       "        wHeight(%2u)                     %5u\n",
			       i, buf[5+4*i] | (buf[6+4*i] << 8),
			       i, buf[7+4*i] | (buf[8+4*i] << 8));
		printf("        bNumCompressionPatterns           %3u\n", m);
		for (i = 0; i < m; i++)
			printf("        bCompression(%2u)                %5u\n",
			       i, buf[6+4*n+i]);
		dump_junk(buf, "        ", 6+4*n+m);
		break;

	case 0x04: /* FORMAT_UNCOMPRESSED */
	case 0x10: /* FORMAT_FRAME_BASED */
		if (buf[2] == 0x04) {
			printf("(FORMAT_UNCOMPRESSED)\n");
			len = 27;
		} else {
			printf("(FORMAT_FRAME_BASED)\n");
			len = 28;
		}
		if (buf[0] < len)
			printf("      Warning: Descriptor too short\n");
		flags = buf[25];
		printf("        bFormatIndex                    %5u\n"
		       "        bNumFrameDescriptors            %5u\n"
		       "        guidFormat                            %s\n"
		       "        bBitsPerPixel                   %5u\n"
		       "        bDefaultFrameIndex              %5u\n"
		       "        bAspectRatioX                   %5u\n"
		       "        bAspectRatioY                   %5u\n"
		       "        bmInterlaceFlags                 0x%02x\n",
		       buf[3], buf[4], get_guid(&buf[5]), buf[21], buf[22],
		       buf[23], buf[24], flags);
		printf("          Interlaced stream or variable: %s\n",
		       (flags & (1 << 0)) ? "Yes" : "No");
		printf("          Fields per frame: %u fields\n",
		       (flags & (1 << 1)) ? 1 : 2);
		printf("          Field 1 first: %s\n",
		       (flags & (1 << 2)) ? "Yes" : "No");
		printf("          Field pattern: ");
		switch ((flags >> 4) & 0x03) {
		case 0:
			printf("Field 1 only\n");
			break;
		case 1:
			printf("Field 2 only\n");
			break;
		case 2:
			printf("Regular pattern of fields 1 and 2\n");
			break;
		case 3:
			printf("Random pattern of fields 1 and 2\n");
			break;
		}
		printf("        bCopyProtect                    %5u\n", buf[26]);
		if (buf[2] == 0x10)
			printf("        bVariableSize                 %5u\n", buf[27]);
		dump_junk(buf, "        ", len);
		break;

	case 0x05: /* FRAME UNCOMPRESSED */
	case 0x07: /* FRAME_MJPEG */
	case 0x11: /* FRAME_FRAME_BASED */
		if (buf[2] == 0x05) {
			printf("(FRAME_UNCOMPRESSED)\n");
			n = 25;
		} else if (buf[2] == 0x07) {
			printf("(FRAME_MJPEG)\n");
			n = 25;
		} else {
			printf("(FRAME_FRAME_BASED)\n");
			n = 21;
		}
		len = (buf[n] != 0) ? (26+buf[n]*4) : 38;
		if (buf[0] < len)
			printf("      Warning: Descriptor too short\n");
		flags = buf[4];
		printf("        bFrameIndex                     %5u\n"
		       "        bmCapabilities                   0x%02x\n",
		       buf[3], flags);
		printf("          Still image %ssupported\n",
		       (flags & (1 << 0)) ? "" : "un");
		if (flags & (1 << 1))
			printf("          Fixed frame-rate\n");
		printf("        wWidth                          %5u\n"
		       "        wHeight                         %5u\n"
		       "        dwMinBitRate                %9u\n"
		       "        dwMaxBitRate                %9u\n",
		       buf[5] | (buf[6] <<  8), buf[7] | (buf[8] << 8),
		       buf[9] | (buf[10] << 8) | (buf[11] << 16) | (buf[12] << 24),
		       buf[13] | (buf[14] << 8) | (buf[15] << 16) | (buf[16] << 24));
		if (buf[2] == 0x11)
			printf("        dwDefaultFrameInterval      %9u\n"
			       "        bFrameIntervalType              %5u\n"
			       "        dwBytesPerLine              %9u\n",
			       buf[17] | (buf[18] << 8) | (buf[19] << 16) | (buf[20] << 24),
			       buf[21],
			       buf[22] | (buf[23] << 8) | (buf[24] << 16) | (buf[25] << 24));
		else
			printf("        dwMaxVideoFrameBufferSize   %9u\n"
			       "        dwDefaultFrameInterval      %9u\n"
			       "        bFrameIntervalType              %5u\n",
			       buf[17] | (buf[18] << 8) | (buf[19] << 16) | (buf[20] << 24),
			       buf[21] | (buf[22] << 8) | (buf[23] << 16) | (buf[24] << 24),
			       buf[25]);
		if (buf[n] == 0)
			printf("        dwMinFrameInterval          %9u\n"
			       "        dwMaxFrameInterval          %9u\n"
			       "        dwFrameIntervalStep         %9u\n",
			       buf[26] | (buf[27] << 8) | (buf[28] << 16) | (buf[29] << 24),
			       buf[30] | (buf[31] << 8) | (buf[32] << 16) | (buf[33] << 24),
			       buf[34] | (buf[35] << 8) | (buf[36] << 16) | (buf[37] << 24));
		else
			for (i = 0; i < buf[n]; i++)
				printf("        dwFrameInterval(%2u)         %9u\n",
				       i, buf[26+4*i] | (buf[27+4*i] << 8) |
				       (buf[28+4*i] << 16) | (buf[29+4*i] << 24));
		dump_junk(buf, "        ", len);
		break;

	case 0x06: /* FORMAT_MJPEG */
		printf("(FORMAT_MJPEG)\n");
		if (buf[0] < 11)
			printf("      Warning: Descriptor too short\n");
		flags = buf[5];
		printf("        bFormatIndex                    %5u\n"
		       "        bNumFrameDescriptors            %5u\n"
		       "        bFlags                          %5u\n",
		       buf[3], buf[4], flags);
		printf("          Fixed-size samples: %s\n",
		       (flags & (1 << 0)) ? "Yes" : "No");
		flags = buf[9];
		printf("        bDefaultFrameIndex              %5u\n"
		       "        bAspectRatioX                   %5u\n"
		       "        bAspectRatioY                   %5u\n"
		       "        bmInterlaceFlags                 0x%02x\n",
		       buf[6], buf[7], buf[8], flags);
		printf("          Interlaced stream or variable: %s\n",
		       (flags & (1 << 0)) ? "Yes" : "No");
		printf("          Fields per frame: %u fields\n",
		       (flags & (1 << 1)) ? 2 : 1);
		printf("          Field 1 first: %s\n",
		       (flags & (1 << 2)) ? "Yes" : "No");
		printf("          Field pattern: ");
		switch ((flags >> 4) & 0x03) {
		case 0:
			printf("Field 1 only\n");
			break;
		case 1:
			printf("Field 2 only\n");
			break;
		case 2:
			printf("Regular pattern of fields 1 and 2\n");
			break;
		case 3:
			printf("Random pattern of fields 1 and 2\n");
			break;
		}
		printf("        bCopyProtect                    %5u\n", buf[10]);
		dump_junk(buf, "        ", 11);
		break;

	case 0x0a: /* FORMAT_MPEG2TS */
		printf("(FORMAT_MPEG2TS)\n");
		len = buf[0] < 23 ? 7 : 23;
		if (buf[0] < len)
			printf("      Warning: Descriptor too short\n");
		printf("        bFormatIndex                    %5u\n"
		       "        bDataOffset                     %5u\n"
		       "        bPacketLength                   %5u\n"
		       "        bStrideLength                   %5u\n",
		       buf[3], buf[4], buf[5], buf[6]);
		if (len > 7)
			printf("        guidStrideFormat                      %s\n",
			       get_guid(&buf[7]));
		dump_junk(buf, "        ", len);
		break;

	case 0x0d: /* COLORFORMAT */
		printf("(COLORFORMAT)\n");
		if (buf[0] < 6)
			printf("      Warning: Descriptor too short\n");
		printf("        bColorPrimaries                 %5u (%s)\n",
		       buf[3], (buf[3] <= 5) ? colorPrims[buf[3]] : "Unknown");
		printf("        bTransferCharacteristics        %5u (%s)\n",
		       buf[4], (buf[4] <= 7) ? transferChars[buf[4]] : "Unknown");
		printf("        bMatrixCoefficients             %5u (%s)\n",
		       buf[5], (buf[5] <= 5) ? matrixCoeffs[buf[5]] : "Unknown");
		dump_junk(buf, "        ", 6);
		break;

	case 0x12: /* FORMAT_STREAM_BASED */
		printf("(FORMAT_STREAM_BASED)\n");
		if (buf[0] != 24)
			printf("      Warning: Incorrect descriptor length\n");

		printf("        bFormatIndex                    %5u\n"
		       "        guidFormat                            %s\n"
		       "        dwPacketLength                %7u\n",
		       buf[3], get_guid(&buf[4]), buf[20]);
		dump_junk(buf, "        ", 24);
		break;

	default:
		printf("        Invalid desc subtype:");
		dump_bytes(buf+3, buf[0]-3);
		break;
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

static void dump_altsetting(libusb_device_handle *dev, const struct libusb_interface_descriptor *interface)
{
	char cls[128], subcls[128], proto[128];
	char *ifstr;

	const unsigned char *buf;
	unsigned size, i;

	get_class_string(cls, sizeof(cls), interface->bInterfaceClass);
	get_subclass_string(subcls, sizeof(subcls), interface->bInterfaceClass, interface->bInterfaceSubClass);
	get_protocol_string(proto, sizeof(proto), interface->bInterfaceClass, interface->bInterfaceSubClass, interface->bInterfaceProtocol);
	ifstr = get_dev_string(dev, interface->iInterface);

	printf("    Interface Descriptor:\n"
	       "      bLength             %5u\n"
	       "      bDescriptorType     %5u\n"
	       "      bInterfaceNumber    %5u\n"
	       "      bAlternateSetting   %5u\n"
	       "      bNumEndpoints       %5u\n"
	       "      bInterfaceClass     %5u %s\n"
	       "      bInterfaceSubClass  %5u %s\n"
	       "      bInterfaceProtocol  %5u %s\n"
	       "      iInterface          %5u %s\n",
	       interface->bLength, interface->bDescriptorType, interface->bInterfaceNumber,
	       interface->bAlternateSetting, interface->bNumEndpoints, interface->bInterfaceClass, cls,
	       interface->bInterfaceSubClass, subcls, interface->bInterfaceProtocol, proto,
	       interface->iInterface, ifstr);

	free(ifstr);

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
}
#endif

void UsbDevice::dump_interface(const struct libusb_interface *interface, vector<string> &intf_info)
{
#if 0
	int i;

	for (i = 0; i < interface->num_altsetting; i++)
		dump_altsetting(dev_handle_, &interface->altsetting[i]);
#endif
}
