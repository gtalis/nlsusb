#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <iostream>
#include <string>
#include <libusb.h>
#include <vector>
#include "names.h"

using namespace std;

class UsbDevice {

	int bus_num_;
	int device_addr_;
	uint16_t id_vendor_;
	uint16_t id_product_;
	string product_name_;
	string vendor_name_;
	libusb_device_handle *dev_handle_;
	struct libusb_device_descriptor descriptor_;

private:
	void dump_device(vector<string> &desc_info);

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

	string getInfoSummary();
	void getInfoDetails(vector<string> &info);
};

#endif