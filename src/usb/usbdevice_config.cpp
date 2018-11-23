#include "usbdevice.h"
#include "names.h"
#include "usbmisc.h"

#include <stdio.h>
#include <string.h>

using namespace std;

void UsbDevice::getConfigsInfo(vector<string> &config_info)
{
	char line[128];

	if (descriptor_.bNumConfigurations) {
		struct libusb_config_descriptor *config;

		int ret = libusb_get_config_descriptor(usb_dev_, 0, &config);
		if (ret) {
			config_info.push_back("Couldn't get configuration descriptor 0, "
					"some information will be missing");
		} else {
			int otg = 0;
			otg = do_otg(config, config_info) || otg;
			libusb_free_config_descriptor(config);
		}

		for (int i = 0; i < descriptor_.bNumConfigurations; ++i) {
			ret = libusb_get_config_descriptor(usb_dev_, i, &config);
			if (ret) {
				snprintf(line, 128, "Couldn't get configuration "
						"descriptor %d, some information will "
						"be missing\n", i);
				config_info.push_back(line);
			} else {
				dump_config(config, config_info);
				libusb_free_config_descriptor(config);
			}
		}
	}	
}

void UsbDevice::dump_config(struct libusb_config_descriptor *config, vector<string> &desc_info)
{

}