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
	
	TheCtx.Clean();
}
