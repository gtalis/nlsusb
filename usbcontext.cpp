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

		snprintf(deviceInfoBuf, 64, "Bus %03d Device %03d: ID %04x:%04x\t%s %s",
				device.getBusNumber(),
				device.getDeviceAddr(),
				device.getIdVendor(),
				device.getIdProduct(),
				device.getVendorName().c_str(),
				device.getProductName().c_str() );
		list.push_back (deviceInfoBuf);
	}
}

void UsbContext::Clean()
{
	names_exit();
	libusb_exit(ctx_);
}