#include "usbdevice.h"
#include "names.h"
#include "usbmisc.h"

#include <stdio.h>
#include <string.h>

using namespace std;

#if 1

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

#if 0
	while (size >= 3) {
		if (buf[0] < 3) {
			printf("buf[0] = %u\n", buf[0]);
			goto out;
		}
		switch (buf[2]) {
		case USB_DC_WIRELESS_USB:
			/* FIXME */
			break;
		case USB_DC_20_EXTENSION:
			dump_usb2_device_capability_desc(buf);
			break;
		case USB_DC_SUPERSPEED:
			dump_ss_device_capability_desc(buf);
			break;
		case USB_DC_SUPERSPEEDPLUS:
			dump_ssp_device_capability_desc(buf);
			break;
		case USB_DC_CONTAINER_ID:
			dump_container_id_device_capability_desc(buf);
			break;
		case USB_DC_PLATFORM:
			dump_platform_device_capability_desc(fd, buf);
			break;
		case USB_DC_BILLBOARD:
			dump_billboard_device_capability_desc(fd, buf);
			break;
		case USB_DC_CONFIGURATION_SUMMARY:
			printf("  Configuration Summary Device Capability:\n");
			desc_dump(fd, desc_usb3_dc_configuration_summary,
					buf, DESC_BUF_LEN_FROM_BUF, 2);
			break;
		default:
			printf("  ** UNRECOGNIZED: ");
			dump_bytes(buf, buf[0]);
			break;
		}
		size -= buf[0];
		buf += buf[0];
	}
#endif

out:
	free(bos_desc);
}
#endif