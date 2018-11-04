#include "usbcontext.h"
#include <list>
#include "mainview.h"

using namespace std;

int main(int argc, char **argv)
{
	UsbContext TheCtx;
	
	TheCtx.Init();
	
	vector<string> usbdevs;
	TheCtx.getUsbDevicesList(usbdevs);
	
	mainview mV;
	
	mV.show(usbdevs);
	
#if 0
	for (string usb : usbdevs) {
		cout << "Usb found : " << usb << endl;
	}
#endif
	
	TheCtx.Clean();
}
