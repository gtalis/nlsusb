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
	int r = libusb_init(NULL);
	if (r < 0)
		return r;
		
	libusb_device **devs;

	ssize_t cnt = libusb_get_device_list(NULL, &devs);
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
	char deviceInfoBuf[64];

	for (UsbDevice device : usb_devices_) {
#if 0
		list.push_back( 
			to_string(device.getBusNumber()) 
			+ "\t" 
			+ to_string(device.getDeviceAddr()) 
			+ "\t"
			+ to_string(device.getIdVendor())
			+ "\t"
			+ to_string(device.getIdProduct())
			+ "\t"
			+ device.getProductName() );
#endif

		snprintf(deviceInfoBuf, 64, "%03d\t%03d\t%04x:%04x\t%s",
				device.getBusNumber(),
				device.getDeviceAddr(),
				device.getIdVendor(),
				device.getIdProduct(),
				device.getProductName().c_str() );
		list.push_back (deviceInfoBuf);
	}
}

void UsbContext::Clean()
{
	libusb_exit(NULL);
}
