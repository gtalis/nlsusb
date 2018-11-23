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

	libusb_device *usb_dev_;

private:
	void dump_device(vector<string> &desc_info);
	int do_wireless(vector<string> &desc_info);
	int do_otg(struct libusb_config_descriptor *config, vector<string> &otg_info);
	void dump_config(struct libusb_config_descriptor *config, vector<string> &config_info);
	void do_hub(vector<string> &hub_info);
	void dump_hub(const char *prefix, const unsigned char *p, vector<string> &hub_info);
	void dump_bos_descriptor(vector<string> &hub_info);
	void do_dualspeed(vector<string> &hub_info);

	void getConfigsInfo(vector<string> &config_info);
	//do_debug(udev);
	//dump_device_status(udev, otg, wireless, desc.bcdUSB >= 0x0300);



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