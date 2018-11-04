#include <iostream>
#include <string>
#include <libusb.h>
#include "names.h"

using namespace std;

class UsbDevice {
public:
	UsbDevice(libusb_device *dev);
	UsbDevice();
	void FillDeviceInfo(libusb_device *dev);
	~UsbDevice();
	
	int getBusNumber() { return bus_num_; }
	int getDeviceAddr() { return device_addr_; }
	int getIdVendor() { return id_vendor_; }
	int getIdProduct() { return id_product_; }
	string getProductName() { return product_name_; }
	string getVendorName() { return vendor_name_; }

private:
	int bus_num_;
	int device_addr_;
	uint16_t id_vendor_;
	uint16_t id_product_;
	string product_name_;
	string vendor_name_;
	string serial_number_;
	libusb_device_handle *dev_handle_;
};
