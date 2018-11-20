#include "usbcontext.h"
#include <list>
#include "mainview.h"

using namespace std;


int main(int argc, char **argv)
{

	UsbContext TheCtx;
	
	TheCtx.Init();
	
	mainview mV;
	
	mV.show(&TheCtx);
	
	TheCtx.Clean();
}
