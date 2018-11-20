#include "usbcontext.h"
#include "string.h"


UsbContext::UsbContext()
{
}

UsbContext::~UsbContext()
{
}


int UsbContext::Init()
{
	/* by default, print names as well as numbers */
	int r = names_init();
	if (r<0) {
		cout << "unable to initialize usb spec" << endl;
		return r;
	}

	r = libusb_init(&ctx_);
	if (r < 0)
		return r;
		
	libusb_device **devs;

	ssize_t cnt = libusb_get_device_list(ctx_, &devs);
	if (cnt < 0)
		return (int) cnt;

	libusb_device *dev;
	int i = 0;

	while ((dev = devs[i++]) != NULL) {
		UsbDevice *usbDev = new UsbDevice(dev);
		if (usbDev)
			usb_devices_.push_back(*usbDev);
	}

	libusb_free_device_list(devs, 1);
}

void UsbContext::getUsbDevicesList(vector<string> &list)
{
	for (UsbDevice device : usb_devices_) {
		list.push_back (device.getInfoSummary());
	}
}

void UsbContext::Clean()
{
	names_exit();
	libusb_exit(ctx_);
}

void UsbContext::getUsbDeviceInfo(int index, vector<string> &list)
{
	usb_devices_[index].getInfoDetails(list);
}