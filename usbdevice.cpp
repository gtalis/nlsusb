#include "usbdevice.h"
#include "names.h"

#include <stdio.h>

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
	char deviceInfoBuf[64];
	snprintf(deviceInfoBuf, 64, "Bus %03d Device %03d: ID %04x:%04x\t%s %s",
			getBusNumber(),
			getDeviceAddr(),
			getIdVendor(),
			getIdProduct(),
			getVendorName().c_str(),
			getProductName().c_str() );

	string info = deviceInfoBuf;
	return info;
}

string UsbDevice::getInfoDetails()
{
	return NULL;
}
