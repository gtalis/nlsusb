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

#ifndef USB_CONTEXT_H
#define USB_CONTEXT_H

#include "usbdevice.h"
#include <list>
#include <vector>
#include <libusb.h>

using namespace std;

class UsbContext {
	vector<UsbDevice> usb_devices_;
	libusb_context *ctx_;

public:
	UsbContext();
	~UsbContext();
	int Init();
	void Clean();
	void getUsbDevicesList(vector<string> &list);
	void getUsbDeviceInfo(int usb_device_index, vector<string> &list);
};

#endif