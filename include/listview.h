#include <ncurses.h>
#include <vector>
#include <string>

//#define DEBUG
#ifdef DEBUG
#include <fstream>
#endif

class ListView {
	WINDOW* win_;
	std::string	name_;
	std::vector<std::string> listItems_;

	int current_index_;
	int win_height_;
	int start_index_;
	bool focused_;

	void ScrollDown();
	void ScrollUp();

#ifdef DEBUG
	std::fstream dbg_file;
#endif

public:
	ListView();
	ListView(const ListView&);
	int Create(WINDOW *parent, std::string name, int nlines, int ncols, int begin_y, int begin_x);
	
	~ListView();
	
	void SetItems(std::vector<std::string> items);
	int getCurrentIndex(void) { return current_index_;}
	
	void Refresh();
	void CursorDown();
	void CursorUp();
	void Select();
	
	void SetFocus(bool enableFocus);
	void ToggleFocus(void) { SetFocus(! focused_); }
	bool GetFocus(void) { return focused_; }
};