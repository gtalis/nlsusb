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

#include "mainview.h"

using namespace std;

#define ERROR	std::cout << __PRETTY_FUNCTION__

mainview::mainview()
	: mCursor (0),
	m_devices_idx(0)
{	
}

mainview::~mainview()
{	
}

void mainview::init()
{
	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();
	clear();

	// Hide cursor
	curs_set(FALSE);

	m_UsbDevices_ListView.Create(stdscr, "Usb_Devices", LINES - 1, COLS/2, 0, 0);
	m_UsbDeviceInfo_ListView.Create(stdscr, "Usb_Device_Info", LINES - 1, COLS/2, 0, COLS/2);

	m_UsbDevices_ListView.SetFocus(true);
	m_UsbDeviceInfo_ListView.SetFocus(false); 
}

void mainview::show(UsbContext *ctx)
{
	// TODO: verify ctx is not NULL
	m_usb_ctx = ctx;

	std::vector<std::string> usbdevs;
	m_usb_ctx->getUsbDevicesList(usbdevs);

	m_UsbDevices_ListView.SetItems(usbdevs);

	std::vector<std::string> usbdevinfo;
	m_usb_ctx->getUsbDeviceInfo(0, usbdevinfo);
	m_UsbDeviceInfo_ListView.SetItems(usbdevinfo);
	show();
}

void mainview::refresh()
{
	m_UsbDevices_ListView.Refresh();
	m_UsbDeviceInfo_ListView.Refresh();

	showHeaderBar();
	showStatusLine();
}

void mainview::scroll_up()
{
	m_UsbDevices_ListView.CursorUp();

	if (m_UsbDevices_ListView.IsFocused()) {
		// Update specific device info pane
		m_devices_idx = m_UsbDevices_ListView.getCurrentIndex();
		std::vector<std::string> usbdevinfo;
		m_usb_ctx->getUsbDeviceInfo(m_devices_idx, usbdevinfo);
		m_UsbDeviceInfo_ListView.ResetCursor();
		m_UsbDeviceInfo_ListView.SetItems(usbdevinfo);
		m_UsbDeviceInfo_ListView.Refresh();
	}

	m_UsbDeviceInfo_ListView.CursorUp();
}

void mainview::scroll_down()
{
	m_UsbDevices_ListView.CursorDown();

	if (m_UsbDevices_ListView.IsFocused()) {
		// Update specific device info pane
		m_devices_idx = m_UsbDevices_ListView.getCurrentIndex();
		std::vector<std::string> usbdevinfo;
		m_usb_ctx->getUsbDeviceInfo(m_devices_idx, usbdevinfo);
		m_UsbDeviceInfo_ListView.ResetCursor();
		m_UsbDeviceInfo_ListView.SetItems(usbdevinfo);
		m_UsbDeviceInfo_ListView.Refresh();
	}
	m_UsbDeviceInfo_ListView.CursorDown();
}


void mainview::toggle_panes()
{
	m_UsbDevices_ListView.ToggleFocus();
	m_UsbDeviceInfo_ListView.ToggleFocus();	
}

void mainview::showHeaderBar()
{
}

void mainview::showStatusLine()
{
	int rows;
	int cols;
	getmaxyx(stdscr,rows,cols);
	wattron(stdscr, A_REVERSE);
	mvwprintw(stdscr, rows - 1 , 1,
		"[F10] Exit\t[TAB] Change window\t\t\t\t\t\t\t");
	wattroff(stdscr, A_REVERSE);

	wrefresh(stdscr);
}

void mainview::show()
{
	init();
	refresh();

	int ch;
	int done = 0;

	while (!done) {
		ch = getch();
		switch (ch) {
		case KEY_UP:
			scroll_up();
			break;
		case KEY_DOWN:
			scroll_down();
			break;
		case '\t':
		case KEY_LEFT:
		case KEY_RIGHT:
			toggle_panes();
			break;
		case 'q':
		case KEY_F(10):
			done = 1;
			break;
		}
		refresh();
	}

	endwin();
}
