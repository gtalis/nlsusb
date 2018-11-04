#include "mainview.h"

using namespace std;

#define ERROR	std::cout << __PRETTY_FUNCTION__

mainview::mainview()
	: mCursor (0)
{
	
}

mainview::~mainview()
{
	
}

void mainview::init()
{
	initscr();			/* Start curses mode 		*/
	raw();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();
	clear();
	
	devices_listview_= subwin(stdscr, LINES, COLS/2, 0, 0);
    device_details_text_view_ = subwin(stdscr, LINES, COLS/2, 0, COLS/2);
    
    box(devices_listview_, ACS_VLINE, ACS_HLINE);
    box(device_details_text_view_, ACS_VLINE, ACS_HLINE);

	// Hide cursor
	curs_set(FALSE);
}

void mainview::show(const vector<string> &linesRef)
{
	mLines = linesRef;
	
	show();
}

void mainview::refresh()
{
	int index = 1;

	//showHeaderBar();

	for (string line : mLines) {
		if (index - 1 == mCursor) {
			wattron(devices_listview_, A_REVERSE);
		}

		mvwprintw(devices_listview_, index, 1, line.c_str());

		if (index - 1 == mCursor) {
			wattroff(devices_listview_, A_REVERSE);
		}

		index++;
	}

	showStatusLine();

    wrefresh(devices_listview_);
    wrefresh(device_details_text_view_);

	::refresh();
}

void mainview::scroll_up()
{
	if (mCursor > 0)
		mCursor--;
}

void mainview::scroll_down()
{
	if (mCursor < mLines.size() - 1)
		mCursor++;
}

void mainview::showHeaderBar()
{
	int rows;
	int cols;
	wattron(devices_listview_, A_REVERSE);
	mvwprintw(devices_listview_, 1 , 1, "BUS\tDEVICE\tPID:VID\t\tProduct Name\t");
	wattroff(devices_listview_, A_REVERSE);
}


void mainview::showStatusLine()
{
	int rows;
	int cols;
	getmaxyx(stdscr,rows,cols);
	wattron(devices_listview_, A_REVERSE);
	mvwprintw(devices_listview_, rows - 2 , 1, "[F10] Exit");
	wattroff(devices_listview_, A_REVERSE);
}

void mainview::show()
{
	init();
	refresh();

	int ch = 0;

	while (ch != KEY_F(10)) {
		ch = getch();
		if (ch == KEY_UP)
			scroll_up();
		else if (ch == KEY_DOWN)
			scroll_down();

		refresh();
	}

	endwin();
}
