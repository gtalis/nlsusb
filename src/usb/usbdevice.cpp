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
	if (descriptor_.bcdUSB == 0x0250) {
		wireless = do_wireless(info);
	}

	get_config_info(info);
	
	if (!dev_handle_)
		return;

	if (descriptor_.bDeviceClass == LIBUSB_CLASS_HUB) {
		do_hub(info);
	}

#if 1	
	if (descriptor_.bcdUSB >= 0x0201) {
		dump_bos_descriptor(info);
	}
#endif


#if 0
	if (descriptor_.bcdUSB == 0x0200) {
		do_dualspeed(info);
	}
	//do_debug(udev);
	//dump_device_status(udev, otg, wireless, desc.bcdUSB >= 0x0300);
	//libusb_close(udev);
#endif	
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

void UsbDevice::dump_bytes(const unsigned char *buf, unsigned int len, vector<string> &info)
{
	unsigned int i;

	char line[128];

	char new_byte[10];
	for (i = 0; i < len; i++) {
		snprintf(new_byte, 10, " %02x", buf[i]);
		strncat(line, new_byte, 128);
	}
	info.push_back(line);
}

void UsbDevice::dump_junk(const unsigned char *buf, const char *indent, unsigned int len, vector<string> &info)
{
	unsigned int i;

	if (buf[0] <= len)
		return;

	char line[128];
	snprintf(line, 64, "%sjunk at descriptor end:", indent);

	char new_byte[10];
	for (i = len; i < buf[0]; i++) {
		snprintf(line, 64, " %02x", buf[i]);
		strncat(line, new_byte, 128);
	}
	info.push_back(line);
}