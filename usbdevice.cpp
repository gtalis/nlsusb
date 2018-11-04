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
	struct libusb_device_descriptor desc;
	int r = libusb_get_device_descriptor(dev, &desc);
	if (r < 0) {
		cerr << "failed to get device descriptor" << endl;
		return;
	}
	
	id_vendor_ = desc.idVendor;
	id_product_ = desc.idProduct;
	bus_num_ = libusb_get_bus_number(dev);
	device_addr_ = libusb_get_device_address(dev);

	char vendor[128], product[128];

	get_vendor_string(vendor, sizeof(vendor), desc.idVendor);
	get_product_string(product, sizeof(product),
			desc.idVendor, desc.idProduct);

	vendor_name_ = vendor;
	product_name_ = product;
}