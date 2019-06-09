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