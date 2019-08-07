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

#include <ncurses.h>
#include <iostream>
#include <vector>
#include <string>
#include <listview.h>
#include <usbcontext.h>

using namespace std;

const int FOCUSED_COLOR_PAIR		= 1;
const int UNFOCUSED_COLOR_PAIR		= 2;
const int STATUS_LINE_COLOR_PAIR	= 3;

#define FOCUSED_FG_COLOR		COLOR_BLACK
#define FOCUSED_BG_COLOR		COLOR_CYAN
#define UNFOCUSED_FG_COLOR		COLOR_BLACK
#define UNFOCUSED_BG_COLOR		COLOR_WHITE

class mainview {
	int mCursor;
	//vector<string> mLines;
	
	//WINDOW	*devices_listview_;
	//WINDOW	*device_details_text_view_;

	ListView m_UsbDevices_ListView;
	ListView m_UsbDeviceInfo_ListView;

	UsbContext *m_usb_ctx;

	int m_devices_idx;

private:
	void show();
	void init(UsbContext *ctx);
	void refresh();
	void refreshUsbDevices();
	void scroll_up();
	void scroll_down();
	void showHeaderBar();
	void showStatusLine();
	void toggle_panes();

static void onUsbHotplugEvent(void *);

public:
	mainview();
	~mainview();
	void show(UsbContext *ctx);
};
