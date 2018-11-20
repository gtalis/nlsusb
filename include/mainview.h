#include <ncurses.h>
#include <iostream>
#include <vector>
#include <string>
#include <listview.h>
#include <usbcontext.h>

using namespace std;

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
	void init();
	void refresh();
	void scroll_up();
	void scroll_down();
	void showHeaderBar();
	void showStatusLine();
	void toggle_panes();

public:
	mainview();
	~mainview();
	void show(UsbContext *ctx);
};
