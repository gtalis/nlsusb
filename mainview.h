#include <ncurses.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

class mainview {
	int mCursor;
	vector<string> mLines;
	
	WINDOW	*devices_listview_;
	WINDOW	*device_details_text_view_;

private:
	void show();
	void init();
	void refresh();
	void scroll_up();
	void scroll_down();
	void showHeaderBar();
	void showStatusLine();

public:
	mainview();
	~mainview();
	void show(const vector<string> &linesRef);
};
