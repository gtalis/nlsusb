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

#include <string>
#include <functional>


using namespace std;

typedef void (*UsbDeviceHotPlugCb)(void *);

class UsbContext {
	vector<UsbDevice> usb_devices_;
	libusb_context *ctx_;
	libusb_hotplug_callback_handle cb_handle_;

	UsbDeviceHotPlugCb hotPlugCb;
	void *user_data;

private:
	int enableHotPlugDetect(bool enable);
	static int hotplug_callback(
		struct libusb_context *ctx,
		struct libusb_device *dev,
		libusb_hotplug_event event,
		void *user_data);
	int refreshUsbDevicesList();
	void invokeHotPlugCallbackFn();


public:
	UsbContext();
	~UsbContext();
	int Init();
	void Clean();
	void getUsbDevicesList(vector<string> &list);
	void getUsbDeviceInfo(int usb_device_index, vector<string> &list);

	void registerHotPlugCallback(UsbDeviceHotPlugCb cb, void *in_arg) { hotPlugCb = cb; this->user_data = in_arg; }
};

#endif
