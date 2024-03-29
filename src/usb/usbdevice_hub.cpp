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

#define CTRL_TIMEOUT	(5*1000)	/* milliseconds */
#define	HUB_STATUS_BYTELEN	3	/* max 3 bytes status = hub + 23 ports */

void UsbDevice::do_hub(vector<string> &hub_info)
{
	unsigned char buf[7 /* base descriptor */
			+ 2 /* bitmasks */ * HUB_STATUS_BYTELEN];
	int i, ret, value;
	unsigned int link_state;

	char hub_info_line[128];
	const char * const link_state_descriptions[] = {
		"U0",
		"U1",
		"U2",
		"suspend",
		"SS.disabled",
		"Rx.Detect",
		"SS.Inactive",
		"Polling",
		"Recovery",
		"Hot Reset",
		"Compliance",
		"Loopback",
	};

	unsigned int speed = descriptor_.bcdUSB;
	unsigned int tt_type = descriptor_.bDeviceProtocol;

	/* USB 3.x hubs have a slightly different descriptor */
	if (speed >= 0x0300)
		value = 0x2A;
	else
		value = 0x29;

	ret = usb_control_msg(dev_handle_,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_DESCRIPTOR,
			value << 8, 0,
			buf, sizeof buf, CTRL_TIMEOUT);
	if (ret < 0) {
		/* Linux returns EHOSTUNREACH for suspended devices */
		if (errno != EHOSTUNREACH) {
			snprintf(hub_info_line, 128, "can't get hub descriptor, %s (%s)\n",
				libusb_error_name(ret), strerror(errno));
			hub_info.push_back(hub_info_line);
		}
		return;
	}
	if (ret < 9 /* at least one port's bitmasks */) {
		snprintf(hub_info_line, 128,
			"incomplete hub descriptor, %d bytes\n",
			ret);
		hub_info.push_back(hub_info_line);
		return;
	}
	dump_hub("", buf, hub_info);

	hub_info.push_back(" Hub Port Status:\n");
	for (i = 0; i < buf[2]; i++) {
		unsigned char status[4];

		ret = usb_control_msg(dev_handle_,
				LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS
					| LIBUSB_RECIPIENT_OTHER,
				LIBUSB_REQUEST_GET_STATUS,
				0, i + 1,
				status, sizeof status,
				CTRL_TIMEOUT);
		if (ret < 0) {
			snprintf(hub_info_line, 128,
				"cannot read port %d status, %s (%d)\n",
				i + 1, strerror(errno), errno);
			hub_info.push_back(hub_info_line);
			break;
		}

		char port_status[128];
		snprintf(port_status, 128,"   Port %d: %02x%02x.%02x%02x", i + 1,
			status[3], status[2],
			status[1], status[0]);

		/* CAPS are used to highlight "transient" states */
		if (speed != 0x0300) {
			char port_state[128];
			snprintf(hub_info_line, 128,"%s%s%s%s%s",
					(status[2] & 0x10) ? " C_RESET" : "",
					(status[2] & 0x08) ? " C_OC" : "",
					(status[2] & 0x04) ? " C_SUSPEND" : "",
					(status[2] & 0x02) ? " C_ENABLE" : "",
					(status[2] & 0x01) ? " C_CONNECT" : "");
			strncat(port_status, hub_info_line, 128);

			char port_state2[128];
			snprintf(hub_info_line, 128,"%s%s%s%s%s%s%s%s%s%s%s",
					(status[1] & 0x10) ? " indicator" : "",
					(status[1] & 0x08) ? " test" : "",
					(status[1] & 0x04) ? " highspeed" : "",
					(status[1] & 0x02) ? " lowspeed" : "",
					(status[1] & 0x01) ? " power" : "",
					(status[0] & 0x20) ? " L1" : "",
					(status[0] & 0x10) ? " RESET" : "",
					(status[0] & 0x08) ? " oc" : "",
					(status[0] & 0x04) ? " suspend" : "",
					(status[0] & 0x02) ? " enable" : "",
					(status[0] & 0x01) ? " connect" : "");
			strncat(port_status, hub_info_line, 128);
			hub_info.push_back(port_status);
		} else {
			char c_link_state[128];
			link_state = ((status[0] & 0xe0) >> 5) +
				((status[1] & 0x1) << 3);
			snprintf(hub_info_line, 128,"%s%s%s%s%s%s",
					(status[2] & 0x80) ? " C_CONFIG_ERROR" : "",
					(status[2] & 0x40) ? " C_LINK_STATE" : "",
					(status[2] & 0x20) ? " C_BH_RESET" : "",
					(status[2] & 0x10) ? " C_RESET" : "",
					(status[2] & 0x08) ? " C_OC" : "",
					(status[2] & 0x01) ? " C_CONNECT" : "");
			strncat(port_status, hub_info_line, 128);
			char c_link_speed[128];
			snprintf(hub_info_line, 128,"%s%s",
					((status[1] & 0x1C) == 0) ? " 5Gbps" : " Unknown Speed",
					(status[1] & 0x02) ? " power" : "");
			strncat(port_status, hub_info_line, 128);

			/* Link state is bits 8:5 */
			if (link_state < (sizeof(link_state_descriptions) /
						sizeof(*link_state_descriptions))) {
				snprintf(hub_info_line, 128, " %s", link_state_descriptions[link_state]);
				strncat(port_status, hub_info_line, 128);
			}
			snprintf(hub_info_line, 128,"%s%s%s%s",
					(status[0] & 0x10) ? " RESET" : "",
					(status[0] & 0x08) ? " oc" : "",
					(status[0] & 0x02) ? " enable" : "",
					(status[0] & 0x01) ? " connect" : "");
			strncat(port_status, hub_info_line, 128);
			hub_info.push_back(port_status);
		}
	}
}

void UsbDevice::dump_hub(const char *prefix, const unsigned char *p, vector<string> &hub_info)
{
	unsigned int l, i, j;
	unsigned int offset;
	unsigned int wHubChar = (p[4] << 8) | p[3];

	unsigned int tt_type = descriptor_.bDeviceProtocol;
	char line[64];
	char extra[10];

	hub_info.push_back(" ");
	snprintf(line, 64, "%sHub Descriptor:\n", prefix);
	hub_info.push_back(line);
	snprintf(line, 64, "%s  bLength             %3u\n", prefix, p[0]);
	hub_info.push_back(line);
	snprintf(line, 64, "%s  bDescriptorType     %3u\n", prefix, p[1]);
	hub_info.push_back(line);
	snprintf(line, 64, "%s  nNbrPorts           %3u\n", prefix, p[2]);
	hub_info.push_back(line);
	snprintf(line, 64, "%s  wHubCharacteristic 0x%04x\n", prefix, wHubChar);
	hub_info.push_back(line);

	switch (wHubChar & 0x03) {
	case 0:
		snprintf(line, 64, "%s    Ganged power switching\n", prefix);
		hub_info.push_back(line);
		break;
	case 1:
		snprintf(line, 64, "%s    Per-port power switching\n", prefix);
		hub_info.push_back(line);
		break;
	default:
		snprintf(line, 64, "%s    No power switching (usb 1.0)\n", prefix);
		hub_info.push_back(line);
		break;
	}
	if (wHubChar & 0x04) {
		snprintf(line, 64, "%s    Compound device\n", prefix);
		hub_info.push_back(line);
	}
	switch ((wHubChar >> 3) & 0x03) {
	case 0:
		snprintf(line, 64, "%s    Ganged overcurrent protection\n", prefix);
		hub_info.push_back(line);
		break;
	case 1:
		snprintf(line, 64, "%s    Per-port overcurrent protection\n", prefix);
		hub_info.push_back(line);
		break;
	default:
		snprintf(line, 64, "%s    No overcurrent protection\n", prefix);
		hub_info.push_back(line);
		break;
	}
	/* USB 3.0 hubs don't have TTs. */
	if (tt_type >= 1 && tt_type < 3) {
		l = (wHubChar >> 5) & 0x03;
		snprintf(line, 64, "%s    TT think time %d FS bits\n", prefix, (l + 1) * 8);
		hub_info.push_back(line);
	}
	/* USB 3.0 hubs don't have port indicators.  Sad face. */
	if (tt_type != 3 && wHubChar & (1<<7)) {
		snprintf(line, 64, "%s    Port indicators\n", prefix);
		hub_info.push_back(line);
	}
	snprintf(line, 64, "%s  bPwrOn2PwrGood      %3u * 2 milli seconds\n", prefix, p[5]);
	hub_info.push_back(line);

	/* USB 3.0 hubs report current in units of aCurrentUnit, or 4 mA */
	if (tt_type == 3) {
		snprintf(line, 64, "%s  bHubContrCurrent   %4u milli Ampere\n",
				prefix, p[6]*4);
		hub_info.push_back(line);
	}
	else {
		snprintf(line, 64, "%s  bHubContrCurrent    %3u milli Ampere\n",
				prefix, p[6]);
		hub_info.push_back(line);
	}

	if (tt_type == 3) {
		snprintf(line, 64, "%s  bHubDecLat          0.%1u micro seconds\n",
				prefix, p[7]);
		hub_info.push_back(line);
		snprintf(line, 64, "%s  wHubDelay          %4u nano seconds\n",
				prefix, (p[8] << 4) +(p[7]));
		hub_info.push_back(line);
		offset = 10;
	} else {
		offset = 7;
	}

	l = (p[2] >> 3) + 1; /* this determines the variable number of bytes following */
	if (l > HUB_STATUS_BYTELEN)
		l = HUB_STATUS_BYTELEN;
	snprintf(line, 64, "%s  DeviceRemovable   ", prefix);
	for (i = 0; i < l; i++) {
		snprintf(extra, 10, " 0x%02x", p[offset+i]);
		strncat(line, extra, 64);
	}
	hub_info.push_back(line);

	if (tt_type != 3) {
		snprintf(line, 64, "%s  PortPwrCtrlMask   ", prefix);
		for (j = 0; j < l; j++) {
			snprintf(extra, 10, " 0x%02x", p[offset+i+j]);
			strncat(line, extra, 64);
		}
		hub_info.push_back(line);
	}
}