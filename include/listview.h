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
#include <vector>
#include <string>

//#define DEBUG
#ifdef DEBUG
#include <fstream>
#endif

typedef struct {
	uint8_t focused;
	uint8_t unfocused;
} Colors_t;

class ListView {
	WINDOW* win_;
	std::string	name_;
	std::vector<std::string> listItems_;

	int current_index_;
	int win_height_;
	int start_index_;
	bool focused_;

	Colors_t colors_;
	uint8_t color_;

	void ScrollDown();
	void ScrollUp();

#ifdef DEBUG
	std::fstream dbg_file;
#endif

public:
	ListView();
	ListView(const ListView&);
	int Create(WINDOW *parent,
		std::string name,
		int nlines,
		int ncols,
		int begin_y,
		int begin_x,
		Colors_t *c = 0);
	
	~ListView();
	
	void SetItems(std::vector<std::string> items);
	int getCurrentIndex(void) { return current_index_;}
	
	void Refresh();
	void CursorDown();
	void CursorUp();
	void Select();
	void ResetCursor();
	
	void SetFocus(bool enableFocus);
	void ToggleFocus(void) { SetFocus(! focused_); }
	bool IsFocused(void) { return focused_; }

	unsigned int GetSize() { return listItems_.size(); }
};
