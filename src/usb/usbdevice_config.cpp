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

void UsbDevice::dump_configs(vector<string> &config_info)
{
	char line[128];
	int ret;
	struct libusb_config_descriptor *config;

	for (int i = 0; i < descriptor_.bNumConfigurations; ++i) {
		ret = libusb_get_config_descriptor(usb_dev_, i, &config);
		if (ret) {
			snprintf(line, 128, "Couldn't get configuration "
					"descriptor %d, some information will "
					"be missing\n", i);
			config_info.push_back(line);
		} else {
			dump_config(config, config_info);
			libusb_free_config_descriptor(config);
		}
	}
}

void UsbDevice::dump_config(struct libusb_config_descriptor *config, vector<string> &config_info)
{
	char *cfg;
	int i;

	unsigned int speed = descriptor_.bcdUSB;

	cfg = get_dev_string(dev_handle_, config->iConfiguration);

	char line[128];
	snprintf(line, 128,"  Configuration Descriptor:\n");
	config_info.push_back(line);
	snprintf(line, 128,"    bLength             %5u\n", config->bLength);
	config_info.push_back(line);
	snprintf(line, 128,"    bDescriptorType     %5u\n", config->bDescriptorType);
	config_info.push_back(line);
	snprintf(line, 128,"    wTotalLength       0x%04x\n", le16_to_cpu(config->wTotalLength));
	config_info.push_back(line);
	snprintf(line, 128,"    bNumInterfaces      %5u\n", config->bNumInterfaces);
	config_info.push_back(line);
	snprintf(line, 128,"    bConfigurationValue %5u\n", config->bConfigurationValue);
	config_info.push_back(line);
	snprintf(line, 128,"    iConfiguration      %5u %s\n", config->iConfiguration, cfg);
	config_info.push_back(line);
	snprintf(line, 128,"    bmAttributes         0x%02x\n", config->bmAttributes);
	config_info.push_back(line);

	free(cfg);

	if (!(config->bmAttributes & 0x80)) {
		snprintf(line, 128,"      (Missing must-be-set bit!)\n");
		config_info.push_back(line);
	}
	if (config->bmAttributes & 0x40) {
		snprintf(line, 128,"      Self Powered\n");
		config_info.push_back(line);
	}
	else {
		snprintf(line, 128,"      (Bus Powered)\n");
		config_info.push_back(line);
	}
	if (config->bmAttributes & 0x20) {
		snprintf(line, 128,"      Remote Wakeup\n");
		config_info.push_back(line);
	}
	if (config->bmAttributes & 0x10) {
		snprintf(line, 128,"      Battery Powered\n");
		config_info.push_back(line);
	}
	snprintf(line, 128, "    MaxPower            %5umA\n", config->MaxPower * (speed >= 0x0300 ? 8 : 2));
	config_info.push_back(line);



	/* avoid re-ordering or hiding descriptors for display */
	if (config->extra_length) {
		int		size = config->extra_length;
		const unsigned char	*buf = config->extra;

		while (size >= 2) {
			if (buf[0] < 2) {
				dump_junk(buf, "        ", size, (char **)&line, 128);
				config_info.push_back(line);
				break;
			}
			switch (buf[1]) {
			case USB_DT_OTG:
				/* handled separately */
				break;
			case USB_DT_INTERFACE_ASSOCIATION:
				dump_association(buf, config_info);
				break;
			case USB_DT_SECURITY:
				dump_security(buf, config_info);
				break;
			case USB_DT_ENCRYPTION_TYPE:
				dump_encryption_type(buf, config_info);
				break;
			default:
				/* often a misplaced class descriptor */
				snprintf(line, 128, "  ** UNRECOGNIZED: ");
				dump_bytes(buf, buf[0], (char **)&line, 128);
				config_info.push_back(line);

				break;
			}
			size -= buf[0];
			buf += buf[0];
		}
	}

#if 0
	for (i = 0 ; i < config->bNumInterfaces ; i++)
		dump_interface(dev, &config->interface[i]);

#endif
}


void UsbDevice::dump_security(const unsigned char *buf, vector<string> &config_info)
{
	char line[128];
	snprintf(line, 128, "    Security Descriptor:\n");
	config_info.push_back(line);
	snprintf(line, 128, "      bLength             %5u\n", buf[0]);
	config_info.push_back(line);
	snprintf(line, 128, "      bDescriptorType     %5u\n", buf[1]);
	config_info.push_back(line);
	snprintf(line, 128, "      wTotalLength       0x%04x\n", (buf[3] << 8 | buf[2]) );
	config_info.push_back(line);
	snprintf(line, 128, "      bNumEncryptionTypes %5u\n", buf[4]);
	config_info.push_back(line);
}

static const char * const encryption_type[] = {
	"UNSECURE",
	"WIRED",
	"CCM_1",
	"RSA_1",
	"RESERVED"
};

void UsbDevice::dump_encryption_type(const unsigned char *buf, vector<string> &config_info)
{
	int b_encryption_type = buf[2] & 0x4;

	char line[128];
	snprintf(line, 128, "    Encryption Type Descriptor:\n");
	config_info.push_back(line);
	snprintf(line, 128, "      bLength             %5u\n", buf[0]);
	config_info.push_back(line);
	snprintf(line, 128, "      bDescriptorType     %5u\n", buf[1]);
	config_info.push_back(line);
	snprintf(line, 128, "      bEncryptionType     %5u %s\n", buf[2], encryption_type[b_encryption_type]);
	config_info.push_back(line);
	snprintf(line, 128, "      bEncryptionValue    %5u\n", buf[3]);
	config_info.push_back(line);
	snprintf(line, 128, "      bAuthKeyIndex       %5u\n", buf[4]);
	config_info.push_back(line);
}

void UsbDevice::dump_association(const unsigned char *buf, vector<string> &config_info)
{
	char cls[128], subcls[128], proto[128];
	char *func;

	get_class_string(cls, sizeof(cls), buf[4]);
	get_subclass_string(subcls, sizeof(subcls), buf[4], buf[5]);
	get_protocol_string(proto, sizeof(proto), buf[4], buf[5], buf[6]);
	func = get_dev_string(dev_handle_, buf[7]);

	char line[128];
	snprintf(line, 128, "    Interface Association:\n");
	config_info.push_back(line);
	snprintf(line, 128, "      bLength             %5u\n", buf[0]);
	config_info.push_back(line);
	snprintf(line, 128, "      bDescriptorType     %5u\n", buf[1]);
	config_info.push_back(line);
	snprintf(line, 128, "      bFirstInterface     %5u\n", buf[2]);
	config_info.push_back(line);
	snprintf(line, 128, "      bInterfaceCount     %5u\n", buf[3]);
	config_info.push_back(line);
	snprintf(line, 128, "      bFunctionClass      %5u %s\n", buf[4], cls);
	config_info.push_back(line);
	snprintf(line, 128, "      bFunctionSubClass   %5u %s\n", buf[5], subcls);
	config_info.push_back(line);
	snprintf(line, 128, "      bFunctionProtocol   %5u %s\n", buf[6], proto);
	config_info.push_back(line);
	snprintf(line, 128, "      iFunction           %5u %s\n", buf[7], func);
	config_info.push_back(line);

	free(func);
}