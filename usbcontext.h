#include "usbdevice.h"
#include <list>
#include <vector>
#include <libusb.h>

using namespace std;

class UsbContext {
	list<UsbDevice> usb_devices_;
	libusb_context *ctx_;

public:
	UsbContext();
	~UsbContext();
	int Init();
	void Clean();
	void getUsbDevicesList(vector<string> &list);
};



