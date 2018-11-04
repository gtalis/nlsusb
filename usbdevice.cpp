#include "usbdevice.h"

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
	
	r = libusb_open(dev, &dev_handle_);
	if (r < 0) {
		cerr << "failed to open device" << endl;
		return;		
	}
	
	unsigned char strDesc[256];
	
	//printf("BUS %03d Device %03d %04x:%04x\t",  bus_num_, device_addr_, id_vendor_ , id_product_);

    r = libusb_get_string_descriptor_ascii(dev_handle_, desc.iManufacturer, strDesc, 256);
    if (r<0) {
    	manufacturer_name_ = "Unknown Manufacturer";
    } else {
    	manufacturer_name_ = (char *)strDesc;
    }
    
    r = libusb_get_string_descriptor_ascii(dev_handle_, desc.iSerialNumber, strDesc, 256);
    if (r<0) {
    	serial_number_ = "Unknown Serial Number";
    } else {
    	serial_number_ = (char *)strDesc;
    }
    
    r = libusb_get_string_descriptor_ascii(dev_handle_, desc.iProduct, strDesc, 256);
    if (r<0) {
    	product_name_ = serial_number_ + manufacturer_name_;
    } else {
    	product_name_ = (char *)strDesc;
    }
    
    //printf("%s %s\n",  manufacturer_name_.c_str(), product_name_.c_str());
    
    libusb_close(dev_handle_);	
}
