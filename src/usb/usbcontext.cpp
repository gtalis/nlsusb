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

#include "usbcontext.h"
#include "string.h"
#include <thread>

#ifdef DEBUG
#include <fstream>
static std::ofstream ofs;
#endif

static std::thread	eventLoopThread_;


UsbContext::UsbContext() :
	hotPlugCb(0), user_data(0)
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

#ifdef DEBUG
	ofs.open ("hotplug.txt", std::ofstream::out | std::ofstream::app);
	if (ofs.is_open()) {
		ofs << "file is opened\n";
	}
#endif

	r = enableHotPlugDetect(true);
	if (r < 0)
		return r;

	return refreshUsbDevicesList();
}

int UsbContext::refreshUsbDevicesList()
{
	libusb_device **devs;

	if (!ctx_)
		return -1;

	ssize_t cnt = libusb_get_device_list(ctx_, &devs);
	if (cnt < 0)
		return (int) cnt;

	libusb_device *dev;
	int i = 0;

	usb_devices_.clear();
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

static void eventLoop()
{
#ifdef DEBUG
	ofs << "Event Loop is started\n";
#endif

	while (1) {
		libusb_handle_events_completed(NULL, NULL);
	}

}

int UsbContext::hotplug_callback(
		struct libusb_context *ctx,
		struct libusb_device *dev,
		libusb_hotplug_event event,
		void *user_data)
{
	UsbContext* context = (UsbContext*) user_data;

#ifdef DEBUG
	if (ofs.is_open()) {
		if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
			ofs << "### LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED ###\n";
		} else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
			ofs << "### LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT ###\n";
		} else {
			ofs << "Unhandled event\n";
		}
	}
#endif

	context->refreshUsbDevicesList();
	context->invokeHotPlugCallbackFn();
	return 0;
}

void UsbContext::invokeHotPlugCallbackFn()
{
	if (hotPlugCb)
		hotPlugCb(user_data);
}

int UsbContext::enableHotPlugDetect(bool enable)
{
	int ret =
	libusb_hotplug_register_callback(NULL,
			(libusb_hotplug_event) (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
			LIBUSB_HOTPLUG_NO_FLAGS,
			LIBUSB_HOTPLUG_MATCH_ANY,
			LIBUSB_HOTPLUG_MATCH_ANY,
			LIBUSB_HOTPLUG_MATCH_ANY,
			hotplug_callback,
			this,
			&cb_handle_);

	if (ret < 0)
		return ret;

	eventLoopThread_ = std::thread(eventLoop);
	return 0;
}

void UsbContext::Clean()
{
	names_exit();
	libusb_exit(ctx_);

	eventLoopThread_.detach();

#ifdef DEBUG
	ofs.close();
#endif
}

void UsbContext::getUsbDeviceInfo(int index, vector<string> &list)
{
	usb_devices_[index].getInfoDetails(list);
}

void registerHotPlugCallback(UsbDeviceHotPlugCb cb, void *in_arg)
{
	hotPlugCb = cb;
	user_data = in_arg;
}