#include "usbdevice.h"
#include "names.h"
#include "usbmisc.h"

#include <stdio.h>
#include <string.h>

using namespace std;

UsbDevice::UsbDevice(libusb_device *dev)
	: dev_handle_(NULL)
{
	FillDeviceInfo(dev);
}

UsbDevice::UsbDevice()
	: dev_handle_(NULL)
{
}

UsbDevice::~UsbDevice()
{
	//if (dev_handle_)
	//	libusb_close(dev_handle_);
}

void UsbDevice::FillDeviceInfo(libusb_device *dev)
{
	int r = libusb_get_device_descriptor(dev, &descriptor_);
	if (r < 0) {
		cerr << "failed to get device descriptor" << endl;
		return;
	}
	
	usb_dev_ = dev;

	id_vendor_ = descriptor_.idVendor;
	id_product_ = descriptor_.idProduct;
	bus_num_ = libusb_get_bus_number(dev);
	device_addr_ = libusb_get_device_address(dev);

	char vendor[128], product[128];

	get_vendor_string(vendor, sizeof(vendor), descriptor_.idVendor);
	get_product_string(product, sizeof(product),
			descriptor_.idVendor, descriptor_.idProduct);

	vendor_name_ = vendor;
	product_name_ = product;

	r = libusb_open(dev, &dev_handle_);
	if (r) {
		dev_handle_ = NULL;
	}
}
 
string UsbDevice::getInfoSummary()
{
	char deviceInfoBuf[128];
	snprintf(deviceInfoBuf, 128, "Bus %03d Device %03d: ID %04x:%04x\t%s %s",
			getBusNumber(),
			getDeviceAddr(),
			getIdVendor(),
			getIdProduct(),
			getVendorName().c_str(),
			getProductName().c_str() );

	string info = deviceInfoBuf;
	return info;
}

void UsbDevice::getInfoDetails(vector<string> &info)
{
	dump_device(info);

	int wireless = 0;
	int otg = 0;

	if (descriptor_.bcdUSB == 0x0250) {
		wireless = do_wireless(info);
	}

	if (descriptor_.bNumConfigurations) {
		struct libusb_config_descriptor *config;

		int ret = libusb_get_config_descriptor(usb_dev_, 0, &config);
		if (ret) {
			info.push_back("Couldn't get configuration descriptor 0, "
					"some information will be missing");
		} else {
			otg = do_otg(config, info) || otg;
			libusb_free_config_descriptor(config);
		}

		dump_configs(info);
	}
	
	if (!dev_handle_)
		return;

	if (descriptor_.bDeviceClass == LIBUSB_CLASS_HUB) {
		do_hub(info);
	}

	if (descriptor_.bcdUSB >= 0x0201) {
		dump_bos_descriptor(info);
	}

	if (descriptor_.bcdUSB == 0x0200) {
		do_dualspeed(info);
	}

	do_debug(info);
	dump_device_status(otg, wireless, descriptor_.bcdUSB >= 0x0300, info);
}


/*
 * General config descriptor dump
 */
void UsbDevice::dump_device(vector<string> &desc_info)
{
	char vendor[128], product[128];
	char cls[128], subcls[128], proto[128];
	char line[128];
	char *mfg, *prod, *serial;

	get_vendor_string(vendor, sizeof(vendor), descriptor_.idVendor);
	get_product_string(product, sizeof(product),
			descriptor_.idVendor, descriptor_.idProduct);
	get_class_string(cls, sizeof(cls), descriptor_.bDeviceClass);
	get_subclass_string(subcls, sizeof(subcls),
			descriptor_.bDeviceClass, descriptor_.bDeviceSubClass);
	get_protocol_string(proto, sizeof(proto), descriptor_.bDeviceClass,
			descriptor_.bDeviceSubClass, descriptor_.bDeviceProtocol);

	mfg = get_dev_string(dev_handle_, descriptor_.iManufacturer);
	prod = get_dev_string(dev_handle_, descriptor_.iProduct);
	serial = get_dev_string(dev_handle_, descriptor_.iSerialNumber);

	if (NULL == dev_handle_) {
		snprintf(line, 128, "Couldn't open device, some information "
			"will be missing");
		desc_info.push_back(line);	
	}

	snprintf(line, 128, "Device Descriptor:");
	desc_info.push_back(line);
	snprintf(line, 128, "bLength             %5u", descriptor_.bLength);
	desc_info.push_back(line);
	snprintf(line, 128, "bDescriptorType     %5u", descriptor_.bDescriptorType);
	desc_info.push_back(line);
	snprintf(line, 128, "bcdUSB              %2x.%02x", descriptor_.bcdUSB >> 8, descriptor_.bcdUSB & 0xff);
	desc_info.push_back(line);
	snprintf(line, 128, "bDeviceClass        %5u %s", descriptor_.bDeviceClass, cls);
	desc_info.push_back(line);
	snprintf(line, 128, "bDeviceSubClass     %5u %s", descriptor_.bDeviceSubClass, subcls);
	desc_info.push_back(line);
	snprintf(line, 128, "bDeviceProtocol     %5u %s", descriptor_.bDeviceProtocol, proto);
	desc_info.push_back(line);
	snprintf(line, 128, "bMaxPacketSize0     %5u", descriptor_.bMaxPacketSize0);
	desc_info.push_back(line);
	snprintf(line, 128, "idVendor           0x%04x %s", descriptor_.idVendor, vendor);
	desc_info.push_back(line);
	snprintf(line, 128, "idProduct          0x%04x %s", descriptor_.idProduct, product);
	desc_info.push_back(line);
	snprintf(line, 128, "bcdDevice           %2x.%02x", descriptor_.bcdDevice >> 8, descriptor_.bcdDevice & 0xff);
	desc_info.push_back(line);
	snprintf(line, 128, "iManufacturer       %5u %s", descriptor_.iManufacturer, mfg);
	desc_info.push_back(line);
	snprintf(line, 128, "iProduct            %5u %s", descriptor_.iProduct, prod);
	desc_info.push_back(line);
	snprintf(line, 128, "iSerial             %5u %s", descriptor_.iSerialNumber, serial);
	desc_info.push_back(line);
	snprintf(line, 128, "bNumConfigurations  %5u", descriptor_.bNumConfigurations);
	desc_info.push_back(line);

	free(mfg);
	free(prod);
	free(serial);
}

int UsbDevice::do_wireless(vector<string> &desc_info)
{
	/* FIXME fetch and dump BOS etc */
	if (dev_handle_)
		return 0;
	return 0;
}


int UsbDevice::do_otg(struct libusb_config_descriptor *config, vector<string> &otg_info)
{
	return 0;
}

void UsbDevice::dump_bytes(
		const unsigned char *buf,
		unsigned int len,
		char **bytes_str,
		unsigned int max_str_len)
{
	char new_byte[10];
	for (int i = 0; i < len; i++) {
		snprintf(new_byte, 10, " %02x", buf[i]);
		strncat(*bytes_str, new_byte, max_str_len);
	}
}

void UsbDevice::dump_junk(
	const unsigned char *buf,
	const char *indent,
	unsigned int len,
	char **junk_str,
	unsigned int max_str_len)
{
	if (buf[0] <= len)
		return;

	snprintf(*junk_str, max_str_len, "%sjunk at descriptor end:", indent);

	char new_byte[10];
	for (int i = len; i < buf[0]; i++) {
		snprintf(new_byte, 10, " %02x", buf[i]);
		strncat(*junk_str, new_byte, max_str_len);
	}
}

void UsbDevice::do_dualspeed(vector<string> &info)
{
	unsigned char buf[10];
	char cls[128], subcls[128], proto[128];
	int ret;

	char line[128];
	ret = usb_control_msg(dev_handle_,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_DESCRIPTOR,
			USB_DT_DEVICE_QUALIFIER << 8, 0,
			buf, sizeof buf, CTRL_TIMEOUT);
	if (ret < 0 && errno != EPIPE)
		perror("can't get device qualifier"); // TODO: add it to info and return gracefully

	/* all dual-speed devices have a qualifier */
	if (ret != sizeof buf
			|| buf[0] != ret
			|| buf[1] != USB_DT_DEVICE_QUALIFIER)
		return;

	get_class_string(cls, sizeof(cls),
			buf[4]);
	get_subclass_string(subcls, sizeof(subcls),
			buf[4], buf[5]);
	get_protocol_string(proto, sizeof(proto),
			buf[4], buf[5], buf[6]);
	snprintf(line, 128, "Device Qualifier (for other device speed):\n");
	info.push_back(line);
	snprintf(line, 128, "  bLength             %5u\n", buf[0]);
	info.push_back(line);
	snprintf(line, 128, "  bDescriptorType     %5u\n", buf[1]);
	info.push_back(line);
	snprintf(line, 128, "  bcdUSB              %2x.%02x\n", buf[3], buf[2]);
	info.push_back(line);
	snprintf(line, 128, "  bDeviceClass        %5u %s\n", buf[4], cls);
	info.push_back(line);
	snprintf(line, 128, "  bDeviceSubClass     %5u %s\n", buf[5], subcls);
	info.push_back(line);
	snprintf(line, 128, "  bDeviceProtocol     %5u %s\n", buf[6], proto);
	info.push_back(line);
	snprintf(line, 128, "  bMaxPacketSize0     %5u\n", buf[7]);
	info.push_back(line);
	snprintf(line, 128, "  bNumConfigurations  %5u\n", buf[8]);
	info.push_back(line);

	/* FIXME also show the OTHER_SPEED_CONFIG descriptors */
}

void UsbDevice::do_debug(vector<string> &info)
{
	unsigned char buf[4];
	int ret;

	char line[128];

	ret = usb_control_msg(dev_handle_,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_DESCRIPTOR,
			USB_DT_DEBUG << 8, 0,
			buf, sizeof buf, CTRL_TIMEOUT);
	if (ret < 0 && errno != EPIPE)
		perror("can't get debug descriptor"); // TODO: add it to info and return gracefully

	/* some high speed devices are also "USB2 debug devices", meaning
	 * you can use them with some EHCI implementations as another kind
	 * of system debug channel:  like JTAG, RS232, or a console.
	 */
	if (ret != sizeof buf
			|| buf[0] != ret
			|| buf[1] != USB_DT_DEBUG)
		return;

	snprintf(line, 128, "Debug descriptor:\n");
	info.push_back(line);
	snprintf(line, 128, "  bLength              %4u\n", buf[0]);
	info.push_back(line);
	snprintf(line, 128, "  bDescriptorType      %4u\n", buf[1]);
	info.push_back(line);
	snprintf(line, 128, "  bDebugInEndpoint     0x%02x\n", buf[2]);
	info.push_back(line);
	snprintf(line, 128, "  bDebugOutEndpoint    0x%02x\n", buf[3]);
	info.push_back(line);
}

void UsbDevice::dump_device_status(int otg, int wireless, int super_speed, vector<string> &status_info)
{
	unsigned char status[8];
	int ret;
	char line[128];

	ret = usb_control_msg(dev_handle_, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
				| LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_STATUS,
			0, 0,
			status, 2,
			CTRL_TIMEOUT);
	if (ret < 0) {
		snprintf(line, 128,
			"cannot read device status, %s (%d)\n",
			strerror(errno), errno);
		status_info.push_back(line);
		return;
	}

	snprintf(line, 128, "Device Status:     0x%02x%02x\n",
			status[1], status[0]);
	status_info.push_back(line);

	if (status[0] & (1 << 0)) {
		snprintf(line, 128, "  Self Powered\n");
		status_info.push_back(line);
	}
	else {
		snprintf(line, 128, "  (Bus Powered)\n");
		status_info.push_back(line);
	}

	if (status[0] & (1 << 1)) {
		snprintf(line, 128, "  Remote Wakeup Enabled\n");
		status_info.push_back(line);
	}

	if (status[0] & (1 << 2) && !super_speed) {
		/* for high speed devices */
		if (!wireless) {
			snprintf(line, 128, "  Test Mode\n");
			status_info.push_back(line);
		}
		/* for devices with Wireless USB support */
		else {
			snprintf(line, 128, "  Battery Powered\n");
			status_info.push_back(line);
		}
	}
	if (super_speed) {
		if (status[0] & (1 << 2)) {
			snprintf(line, 128, "  U1 Enabled\n");
			status_info.push_back(line);
		}
		if (status[0] & (1 << 3)) {
			snprintf(line, 128, "  U2 Enabled\n");
			status_info.push_back(line);
		}
		if (status[0] & (1 << 4)) {
			snprintf(line, 128, "  Latency Tolerance Messaging (LTM) Enabled\n");
			status_info.push_back(line);
		}
	}
	/* if both HOST and DEVICE support OTG */
	if (otg) {
		if (status[0] & (1 << 3)) {
			snprintf(line, 128, "  HNP Enabled\n");
			status_info.push_back(line);
		}
		if (status[0] & (1 << 4)) {
			snprintf(line, 128, "  HNP Capable\n");
			status_info.push_back(line);
		}
		if (status[0] & (1 << 5)) {
			snprintf(line, 128, "  ALT port is HNP Capable\n");
			status_info.push_back(line);
		}
	}
	/* for high speed devices with debug descriptors */
	if (status[0] & (1 << 6)) {
		snprintf(line, 128, "  Debug Mode\n");
		status_info.push_back(line);
	}

	if (!wireless)
		return;

	/* Wireless USB exposes FIVE different types of device status,
	 * accessed by distinct wIndex values.
	 */
	ret = usb_control_msg(dev_handle_, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
				| LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_STATUS,
			0, 1 /* wireless status */,
			status, 1,
			CTRL_TIMEOUT);
	if (ret < 0) {
		snprintf(line, 128,
			"cannot read wireless %s, %s (%d)\n",
			"status",
			strerror(errno), errno);
		status_info.push_back(line);
		return;
	}
	snprintf(line, 128, "Wireless Status:     0x%02x\n", status[0]);
	if (status[0] & (1 << 0)) {
		snprintf(line, 128, "  TX Drp IE\n");
		status_info.push_back(line);
	}
	if (status[0] & (1 << 1)) {
		snprintf(line, 128, "  Transmit Packet\n");
		status_info.push_back(line);
	}
	if (status[0] & (1 << 2)) {
		snprintf(line, 128, "  Count Packets\n");
		status_info.push_back(line);
	}
	if (status[0] & (1 << 3)) {
		snprintf(line, 128, "  Capture Packet\n");
		status_info.push_back(line);
	}

	ret = usb_control_msg(dev_handle_, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
				| LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_STATUS,
			0, 2 /* Channel Info */,
			status, 1,
			CTRL_TIMEOUT);
	if (ret < 0) {
		snprintf(line, 128,
			"cannot read wireless %s, %s (%d)\n",
			"channel info",
			strerror(errno), errno);
		status_info.push_back(line);
		return;
	}
	snprintf(line, 128, "Channel Info:        0x%02x\n", status[0]);
	status_info.push_back(line);

	/* 3=Received data: many bytes, for count packets or capture packet */

	ret = usb_control_msg(dev_handle_, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
				| LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_STATUS,
			0, 3 /* MAS Availability */,
			status, 8,
			CTRL_TIMEOUT);
	if (ret < 0) {
		snprintf(line, 128,
			"cannot read wireless %s, %s (%d)\n",
			"MAS info",
			strerror(errno), errno);
		status_info.push_back(line);
		return;
	}
	snprintf(line, 128, "MAS Availability:    ");
	dump_bytes(status, 8, (char **)&line, 128);
	status_info.push_back(line);

	ret = usb_control_msg(dev_handle_, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
				| LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_STATUS,
			0, 5 /* Current Transmit Power */,
			status, 2,
			CTRL_TIMEOUT);
	if (ret < 0) {
		snprintf(line, 128,
			"cannot read wireless %s, %s (%d)\n",
			"transmit power",
			strerror(errno), errno);
		status_info.push_back(line);
		return;
	}
	snprintf(line, 128, "Transmit Power:\n");
	status_info.push_back(line);
	snprintf(line, 128, " TxNotification:     0x%02x\n", status[0]);
	status_info.push_back(line);
	snprintf(line, 128, " TxBeacon:     :     0x%02x\n", status[1]);
	status_info.push_back(line);
}


//*****  helper functions
const char *get_guid(const unsigned char *buf)
{
	static char guid[39];

	/* NOTE:  see RFC 4122 for more information about GUID/UUID
	 * structure.  The first fields fields are historically big
	 * endian numbers, dating from Apollo mc68000 workstations.
	 */
	snprintf(guid, 39, "{%02x%02x%02x%02x"
			"-%02x%02x"
			"-%02x%02x"
			"-%02x%02x"
			"-%02x%02x%02x%02x%02x%02x}",
	       buf[3], buf[2], buf[1], buf[0],
	       buf[5], buf[4],
	       buf[7], buf[6],
	       buf[8], buf[9],
	       buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
	return guid;
}

unsigned int convert_le_u32 (const unsigned char *buf)
{
	return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

unsigned int convert_le_u16 (const unsigned char *buf)
{
	return buf[0] | (buf[1] << 8);
}