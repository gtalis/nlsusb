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