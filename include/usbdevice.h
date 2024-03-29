/*
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

#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <iostream>
#include <string>
#include <libusb.h>
#include <vector>
#include "names.h"

#include "usb-spec.h"

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
	void dump_bytes(
		const unsigned char *buf,
		unsigned int len,
		char **bytes_str,
		unsigned int max_str_len);
	void dump_junk(
		const unsigned char *buf,
		const char *indent,
		unsigned int len,
		char **junk_str,
		unsigned int max_str_len);

	void dump_device(vector<string> &desc_info);
	int do_wireless(vector<string> &desc_info);
	int do_otg(struct libusb_config_descriptor *config, vector<string> &otg_info);
	void dump_config(struct libusb_config_descriptor *config, vector<string> &config_info);
	void dump_interface(const struct libusb_interface *interface, vector<string> &intf_info);
	void dump_altsetting(const struct libusb_interface_descriptor *interface, vector<string> &intf_info);

	void dump_dfu_interface(const unsigned char *buf, vector<string> &intf_info);
	void dump_ccid_device(const unsigned char *buf, vector<string> &intf_info);

	void dump_hid_device(
			    const struct libusb_interface_descriptor *interface,
			    const unsigned char *buf,
			    vector<string> &intf_info);

	void do_hub(vector<string> &hub_info);
	void dump_hub(const char *prefix, const unsigned char *p, vector<string> &hub_info);
	void dump_bos_descriptor(vector<string> &bos_info);
	void do_dualspeed(vector<string> &info);
	void do_debug(vector<string> &info);

	void dump_configs(vector<string> &config_info);
	void dump_device_status(int otg, int wireless, int super_speed, vector<string> &status_info);

	void dump_security(const unsigned char *buf, vector<string> &info);

	void dump_encryption_type(const unsigned char *buf, vector<string> &info);
	void dump_association(const unsigned char *buf, vector<string> &info);


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

// helper function
inline int typesafe_control_msg(libusb_device_handle *dev,
	unsigned char requesttype, unsigned char request,
	int value, int idx,
	unsigned char *bytes, unsigned size, int timeout)
{
	int ret = libusb_control_transfer(dev, requesttype, request, value,
					idx, bytes, size, timeout);

	return ret;
}

#define usb_control_msg		typesafe_control_msg
#define CTRL_TIMEOUT	(5*1000)	/* milliseconds */

#define le16_to_cpu(x) libusb_cpu_to_le16(libusb_cpu_to_le16(x))

const char *get_guid(const unsigned char *buf);
unsigned int convert_le_u32 (const unsigned char *buf);
unsigned int convert_le_u16 (const unsigned char *buf);

#endif