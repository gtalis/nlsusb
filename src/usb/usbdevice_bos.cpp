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

static void dump_usb2_device_capability_desc(unsigned char *buf, vector<string> &bos_info);
static void dump_ss_device_capability_desc(unsigned char *buf, vector<string> &bos_info);
static void dump_ssp_device_capability_desc(unsigned char *buf, vector<string> &bos_info);
static void dump_container_id_device_capability_desc(unsigned char *buf, vector<string> &bos_info);
static char *get_webusb_url(libusb_device_handle *fd, u_int8_t vendor_req, u_int8_t id);
static void dump_platform_device_capability_desc(libusb_device_handle *fd, unsigned char *buf, vector<string> &bos_info);
static void dump_billboard_device_capability_desc(libusb_device_handle *dev, unsigned char *buf, vector<string> &bos_info);

static const char * const vconn_power[] = {
	"1W",
	"1.5W",
	"2W",
	"3W",
	"4W",
	"5W",
	"6W",
	"reserved"
};

static const char * const alt_mode_state[] = {
	"Unspecified Error",
	"Alternate Mode configuration not attempted",
	"Alternate Mode configuration attempted but unsuccessful",
	"Alternate Mode configuration successful"
};

void UsbDevice::dump_bos_descriptor(vector<string> &bos_info)
{
	/* Total length of BOS descriptors varies.
	 * Read first static 5 bytes which include the total length before
	 * allocating and reading the full BOS
	 */

	unsigned char bos_desc_static[5];
	unsigned char *bos_desc;
	unsigned int bos_desc_size;
	int size, ret;
	unsigned char *buf;

	char line[64];

	/* Get the first 5 bytes to get the wTotalLength field */
	ret = usb_control_msg(dev_handle_,
			LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_DESCRIPTOR,
			USB_DT_BOS << 8, 0,
			bos_desc_static, 5, CTRL_TIMEOUT);
	if (ret <= 0)
		return;
	else if (bos_desc_static[0] != 5 || bos_desc_static[1] != USB_DT_BOS)
		return;

	bos_desc_size = bos_desc_static[2] + (bos_desc_static[3] << 8);

	bos_info.push_back(" ");
	bos_info.push_back("Binary Object Store Descriptor:\n");
	snprintf(line, 64, "  bLength             %5u", bos_desc_static[0]);
	bos_info.push_back(line);
	snprintf(line, 64, "  bDescriptorType     %5u", bos_desc_static[1]);
	bos_info.push_back(line);
	snprintf(line, 64, "  wTotalLength        0x%04x", bos_desc_size);
	bos_info.push_back(line);
	snprintf(line, 64, "  bNumDeviceCaps      %5u", bos_desc_static[4]);
	bos_info.push_back(line);

	if (bos_desc_size <= 5) {
		if (bos_desc_static[4] > 0)
			bos_info.push_back("Couldn't get "
					"device capability descriptors");
		return;
	}
	bos_desc = (unsigned char *)malloc(bos_desc_size);
	if (!bos_desc)
		return;
	memset(bos_desc, 0, bos_desc_size);

	ret = usb_control_msg(dev_handle_,
			LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_DESCRIPTOR,
			USB_DT_BOS << 8, 0,
			bos_desc, bos_desc_size, CTRL_TIMEOUT);
	if (ret < 0) {
		bos_info.push_back("Couldn't get device capability descriptors");
		goto out;
	}

	size = bos_desc_size - 5;
	buf = &bos_desc[5];

	while (size >= 3) {
		if (buf[0] < 3) {
			snprintf(line, 128, "buf[0] = %u\n", buf[0]);
			bos_info.push_back(line);
			goto out;
		}
		switch (buf[2]) {
		case USB_DC_WIRELESS_USB:
			/* FIXME */
			break;
		case USB_DC_20_EXTENSION:
			dump_usb2_device_capability_desc(buf, bos_info);
			break;
		case USB_DC_SUPERSPEED:
			dump_ss_device_capability_desc(buf, bos_info);
			break;
		case USB_DC_SUPERSPEEDPLUS:
			dump_ssp_device_capability_desc(buf, bos_info);
			break;
		case USB_DC_CONTAINER_ID:
			dump_container_id_device_capability_desc(buf, bos_info);
			break;
		case USB_DC_PLATFORM:
			dump_platform_device_capability_desc(dev_handle_, buf, bos_info);
			break;
		case USB_DC_BILLBOARD:
			dump_billboard_device_capability_desc(dev_handle_, buf, bos_info);
			break;
#if 0 //TODO
		case USB_DC_CONFIGURATION_SUMMARY:
			snprintf(line, 128, "  Configuration Summary Device Capability:\n");
			desc_dump(fd, desc_usb3_dc_configuration_summary,
					buf, DESC_BUF_LEN_FROM_BUF, 2);
			break;
#endif
		default:
			snprintf(line, 128, "  ** UNRECOGNIZED: ");
			dump_bytes(buf, buf[0], (char **)&line, 128);
			bos_info.push_back(line);
			break;
		}
		size -= buf[0];
		buf += buf[0];
	}

out:
	free(bos_desc);
}


static void dump_usb2_device_capability_desc(unsigned char *buf, vector<string> &bos_info)
{
	unsigned int wide;
	char line[128];

	wide = buf[3] + (buf[4] << 8) +
		(buf[5] << 16) + (buf[6] << 24);
	snprintf(line, 128, "  USB 2.0 Extension Device Capability:\n");
	bos_info.push_back(line);
	snprintf(line, 128, "    bLength             %5u\n", buf[0]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bDescriptorType     %5u\n", buf[1]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bDevCapabilityType  %5u\n", buf[2]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bmAttributes   0x%08x\n", wide);
	bos_info.push_back(line);

	if (!(wide & 0x02)){
		snprintf(line, 128, "      (Missing must-be-set LPM bit!)\n");
		bos_info.push_back(line);
	}
	else if (!(wide & 0x04)) {
		snprintf(line, 128, "      HIRD Link Power Management (LPM)"
				" Supported\n");
		bos_info.push_back(line);
	}
	else {
		snprintf(line, 128, "      BESL Link Power Management (LPM)"
				" Supported\n");
		bos_info.push_back(line);
		if (wide & 0x08) {
			snprintf(line, 128, "    BESL value    %5u us \n", wide & 0xf00);
			bos_info.push_back(line);
		}
		if (wide & 0x10) {
			snprintf(line, 128, "    Deep BESL value    %5u us \n",
					wide & 0xf000);
			bos_info.push_back(line);
		}
	}
}

static void dump_ss_device_capability_desc(unsigned char *buf, vector<string> &bos_info)
{
	char line[128];

	if (buf[0] < 10) {
		snprintf(line, 128, "  Bad SuperSpeed USB Device Capability descriptor.\n");
		bos_info.push_back(line);
		return;
	}
	snprintf(line, 128, "  SuperSpeed USB Device Capability:\n");
	bos_info.push_back(line);
	snprintf(line, 128, "    bLength             %5u\n", buf[0]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bDescriptorType     %5u\n", buf[1]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bDevCapabilityType  %5u\n", buf[2]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bmAttributes         0x%02x\n", buf[3]);
	bos_info.push_back(line);


	if (buf[3] & 0x02) {
		snprintf(line, 128, "      Latency Tolerance Messages (LTM)"
				" Supported\n");
		bos_info.push_back(line);
	}
	snprintf(line, 128, "    wSpeedsSupported   0x%02x%02x\n", buf[5], buf[4]);
	bos_info.push_back(line);

	if (buf[4] & (1 << 0)) {
		snprintf(line, 128, "      Device can operate at Low Speed (1Mbps)\n");
		bos_info.push_back(line);
	}
	if (buf[4] & (1 << 1)) {
		snprintf(line, 128, "      Device can operate at Full Speed (12Mbps)\n");
		bos_info.push_back(line);
	}
	if (buf[4] & (1 << 2)) {
		snprintf(line, 128, "      Device can operate at High Speed (480Mbps)\n");
		bos_info.push_back(line);
	}
	if (buf[4] & (1 << 3)) {
		snprintf(line, 128, "      Device can operate at SuperSpeed (5Gbps)\n");
		bos_info.push_back(line);
	}

	snprintf(line, 128, "    bFunctionalitySupport %3u\n", buf[6]);
	bos_info.push_back(line);

	switch(buf[6]) {
	case 0:
		snprintf(line, 128, "      Lowest fully-functional device speed is "
				"Low Speed (1Mbps)\n");
		bos_info.push_back(line);
		break;
	case 1:
		snprintf(line, 128, "      Lowest fully-functional device speed is "
				"Full Speed (12Mbps)\n");
		bos_info.push_back(line);
		break;
	case 2:
		snprintf(line, 128, "      Lowest fully-functional device speed is "
				"High Speed (480Mbps)\n");
		bos_info.push_back(line);
		break;
	case 3:
		snprintf(line, 128, "      Lowest fully-functional device speed is "
				"SuperSpeed (5Gbps)\n");
		bos_info.push_back(line);
		break;
	default:
		snprintf(line, 128, "      Lowest fully-functional device speed is "
				"at an unknown speed!\n");
		bos_info.push_back(line);
		break;
	}
	snprintf(line, 128, "    bU1DevExitLat        %4u micro seconds\n", buf[7]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bU2DevExitLat    %8u micro seconds\n", buf[8] + (buf[9] << 8));
	bos_info.push_back(line);
}

static void dump_ssp_device_capability_desc(unsigned char *buf, vector<string> &bos_info)
{
	int i;
	unsigned int bm_attr, ss_attr;
	char bitrate_prefix[] = " KMG";
	char line[128];

	if (buf[0] < 12) {
		snprintf(line, 128, "  Bad SuperSpeedPlus USB Device Capability descriptor.\n");
		bos_info.push_back(line);
		return;
	}

	bm_attr = convert_le_u32(buf + 4);
	snprintf(line, 128, "  SuperSpeedPlus USB Device Capability:\n");
	bos_info.push_back(line);
	snprintf(line, 128, "    bLength             %5u\n", buf[0]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bDescriptorType     %5u\n", buf[1]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bDevCapabilityType  %5u\n", buf[2]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bmAttributes         0x%08x\n", bm_attr);
	bos_info.push_back(line);

	snprintf(line, 128, "      Sublink Speed Attribute count %u\n", buf[4] & 0x1f);
	bos_info.push_back(line);
	snprintf(line, 128, "      Sublink Speed ID count %u\n", (bm_attr >> 5) & 0xf);
	bos_info.push_back(line);
	snprintf(line, 128, "    wFunctionalitySupport   0x%02x%02x\n", buf[9], buf[8]);
	bos_info.push_back(line);

	for (i = 0; i <= (buf[4] & 0x1f); i++) {
		ss_attr = convert_le_u32(buf + 12 + (i * 4));
		snprintf(line, 128, "    bmSublinkSpeedAttr[%u]   0x%08x\n", i, ss_attr);
		bos_info.push_back(line);
		snprintf(line, 128, "      Speed Attribute ID: %u %u%cb/s %s %s SuperSpeed%s\n",
		       ss_attr & 0x0f,
		       ss_attr >> 16,
		       (bitrate_prefix[((ss_attr >> 4) & 0x3)]),
		       (ss_attr & 0x40)? "Asymmetric" : "Symmetric",
		       (ss_attr & 0x80)? "TX" : "RX",
		       (ss_attr & 0x4000)? "Plus": "" );
		bos_info.push_back(line);
	}
}

static void dump_container_id_device_capability_desc(unsigned char *buf, vector<string> &bos_info)
{
	char line[128];
	if (buf[0] < 20) {
		snprintf(line, 128, "  Bad Container ID Device Capability descriptor.\n");
		bos_info.push_back(line);
		return;
	}
	snprintf(line, 128, "  Container ID Device Capability:\n");
	bos_info.push_back(line);
	snprintf(line, 128, "    bLength             %5u\n", buf[0]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bDescriptorType     %5u\n", buf[1]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bDevCapabilityType  %5u\n", buf[2]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bReserved           %5u\n", buf[3]);
	bos_info.push_back(line);
	snprintf(line, 128, "    ContainerID             %s\n",
			get_guid(&buf[4]));
	bos_info.push_back(line);
}

static char *get_webusb_url(libusb_device_handle *fd, u_int8_t vendor_req, u_int8_t id)
{
	unsigned char url_buf[255];
	char scheme[10];
	char *url, *chr;
	unsigned char i;
	int ret;

	ret = usb_control_msg(fd,
			LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_DEVICE | LIBUSB_REQUEST_TYPE_VENDOR,
			vendor_req, id, WEBUSB_GET_URL,
			url_buf, sizeof(url_buf), CTRL_TIMEOUT);
	if (ret <= 0)
		return strdup("");
	else if (url_buf[0] <= 3 || url_buf[1] != USB_DT_WEBUSB_URL || ret != url_buf[0])
		return strdup("");

	switch (url_buf[2]) {
	case 0:
		strncpy(scheme, "http://", 10);
		break;
	case 1:
		strncpy(scheme, "https://", 10);
		break;
	case 255:
		strncpy(scheme, " ", 10);
		//scheme = "";
		break;
	default:
		//snprintf(line, 128, "Bad URL scheme.\n");
		return strdup("");
	}
	url = (char *)malloc(strlen(scheme) + (url_buf[0] - 3)  + 1);
	if (!url)
		return strdup("");
	strcpy(url, scheme);
	chr = url + strlen(scheme);
	for (i = 3; i < url_buf[0]; i++)
		/* crude UTF-8 to ASCII conversion */
		if (url_buf[i] < 0x80)
			*chr++ = url_buf[i];
	*chr = '\0';

	return url;
}

static void dump_platform_device_capability_desc(libusb_device_handle *fd, unsigned char *buf, vector<string> &bos_info)
{
	unsigned char desc_len = buf[0];
	unsigned char cap_data_len = desc_len - 20;
	unsigned char i;
	const char *guid;
	char line[128];

	if (desc_len < 20) {
		snprintf(line, 128, "  Bad Platform Device Capability descriptor.\n");
		bos_info.push_back(line);
		return;
	}

	snprintf(line, 128, "  Platform Device Capability:\n");
	bos_info.push_back(line);
	snprintf(line, 128, "    bLength             %5u\n", buf[0]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bDescriptorType     %5u\n", buf[1]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bDevCapabilityType  %5u\n", buf[2]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bReserved           %5u\n", buf[3]);
	bos_info.push_back(line);

	guid = get_guid(&buf[4]);
	snprintf(line, 128, "    PlatformCapabilityUUID    %s\n", guid);
	bos_info.push_back(line);

	if (!strcmp(WEBUSB_GUID , guid) && desc_len == 24) {
		/* WebUSB platform descriptor */
		char *url = get_webusb_url(fd, buf[22], buf[23]);
		snprintf(line, 128, "      WebUSB:\n");
		bos_info.push_back(line);
		snprintf(line, 128, "        bcdVersion   %2x.%02x\n", buf[21], buf[20]);
		bos_info.push_back(line);
		snprintf(line, 128, "        bVendorCode  %5u\n", buf[22]);
		bos_info.push_back(line);
		snprintf(line, 128, "        iLandingPage %5u %s\n", buf[23], url);
		bos_info.push_back(line);
		free(url);
		return;
	}

	for (i = 0; i < cap_data_len; i++) {
		snprintf(line, 128, "    CapabilityData[%u]    0x%02x\n", i, buf[20 + i]);
		bos_info.push_back(line);
	}
}

static void dump_billboard_device_capability_desc(libusb_device_handle *dev, unsigned char *buf, vector<string> &bos_info)
{
	char *url, *alt_mode_str;
	int w_vconn_power, alt_mode, i, svid, state;
	const char *vconn;
	unsigned char *bmConfigured;
	char line[128];	

	if (buf[0] < 48) {
		snprintf(line, 128, "  Bad Billboard Capability descriptor.\n");
		bos_info.push_back(line);
		return;
	}

	if (buf[4] > BILLBOARD_MAX_NUM_ALT_MODE) {
		snprintf(line, 128, "  Invalid value for bNumberOfAlternateModes.\n");
		bos_info.push_back(line);
		return;
	}

	if (buf[0] < (44 + buf[4] * 4)) {
		snprintf(line, 128, "  bLength does not match with bNumberOfAlternateModes.\n");
		bos_info.push_back(line);
		return;
	}

	url = get_dev_string(dev, buf[3]);
	w_vconn_power = convert_le_u16(buf+6);
	if (w_vconn_power & (1 << 15)) {
		vconn = "VCONN power not required";
	} else if (w_vconn_power < 7) {
		vconn = vconn_power[w_vconn_power & 0x7];
	} else {
		vconn = "reserved";
	}
	snprintf(line, 128, "  Billboard Capability:\n");
	bos_info.push_back(line);
	snprintf(line, 128, "    bLength                 %5u\n", buf[0]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bDescriptorType         %5u\n", buf[0]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bDevCapabilityType      %5u\n", buf[0]);
	bos_info.push_back(line);
	snprintf(line, 128, "    iAddtionalInfoURL       %5u %s\n", buf[3], url);
	bos_info.push_back(line);
	snprintf(line, 128, "    bNumberOfAlternateModes %5u\n", buf[4]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bPreferredAlternateMode %5u\n", buf[5]);
	bos_info.push_back(line);
	snprintf(line, 128, "    VCONN Power             %5u %s\n", w_vconn_power, vconn);
	bos_info.push_back(line);

	bmConfigured = &buf[8];

#if 0
	//TODO: how to invoke dump_bytes from here?
	snprintf(line, 128, "    bmConfigured               ");
	char bm_config_bytes[64];
	dump_bytes(bmConfigured, 32, (char **)&bm_config_bytes, 64);
	strncat(line, bm_config_bytes, 128);
	bos_info.push_back(line);
#endif

	snprintf(line, 128, "    bcdVersion              %2x.%02x\n", (buf[41] == 0) ? 1 : buf[41], buf[40]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bAdditionalFailureInfo  %5u\n", buf[42]);
	bos_info.push_back(line);
	snprintf(line, 128, "    bReserved               %5u\n", buf[43]);
	bos_info.push_back(line);

	snprintf(line, 128, "    Alternate Modes supported by Device Container:\n");
	bos_info.push_back(line);
	i = 44; /* Alternate mode 0 starts at index 44 */
	for (alt_mode = 0; alt_mode < buf[4]; alt_mode++) {
		svid = convert_le_u16(buf+i);
		alt_mode_str = get_dev_string(dev, buf[i+3]);
		state = ((bmConfigured[alt_mode >> 2]) >> ((alt_mode & 0x3) << 1)) & 0x3;
		snprintf(line, 128, "    Alternate Mode %d : %s\n", alt_mode, alt_mode_state[state]);
		bos_info.push_back(line);
		snprintf(line, 128, "      wSVID[%d]                    0x%04X\n", alt_mode, svid);
		bos_info.push_back(line);
		snprintf(line, 128, "      bAlternateMode[%d]       %5u\n", alt_mode, buf[i+2]);
		bos_info.push_back(line);
		snprintf(line, 128, "      iAlternateModeString[%d] %5u %s\n", alt_mode, buf[i+3], alt_mode_str);
		bos_info.push_back(line);
		free(alt_mode_str);
		i += 4;
	}

	free (url);
}