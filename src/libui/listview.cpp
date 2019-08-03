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

#include "listview.h"
#include <iostream>

ListView::ListView()
	: win_(NULL),
	current_index_(0),
	start_index_(0),
	focused_(false)
{
}


ListView::ListView(const ListView& lv)
{
	win_ = lv.win_;
	name_= lv.name_;
	listItems_= lv.listItems_;
	current_index_= lv.current_index_;
	win_height_= lv.win_height_;
	start_index_= lv.start_index_;
	focused_= lv.focused_;

#ifdef DEBUG
	std::string dbg_filename = name_ + "_listview_debug.txt";
	dbg_file.open (dbg_filename, std::fstream::out | std::fstream::app);
#endif
}

int
ListView::Create(WINDOW *parent,
		std::string name,
		int nlines,
		int ncols,
		int begin_y,
		int begin_x,
		Colors_t *c)
{
	if (win_) {
		return 0;
	}

	win_ =
		subwin(parent, nlines, ncols, begin_y, begin_x);
		
	if (win_ == NULL) {
		std::cerr << " Failed to create listview " << name << std::endl;
		return -1;
	}
	
    box(win_, ACS_VLINE, ACS_HLINE);
	
	name_ = name;

	if (c) {
		colors_.focused = c->focused;
		colors_.unfocused = c->unfocused;
	}

	win_height_ = nlines - 2; // "box" uses 2 lines
	
#ifdef DEBUG
	std::string dbg_filename = name + "_listview_debug.txt";
	dbg_file.open (dbg_filename, std::fstream::out);
#endif	
	
	return 0;
}
	
ListView::~ListView()
{
#if 0
	if (win_) {
		delwin(win_);
	}
#endif
	
#ifdef DEBUG
	if (dbg_file.is_open()) {
		dbg_file.close ();
	}
#endif	
}

void ListView::SetItems(std::vector<std::string> items)
{
	listItems_ = items;
	
#ifdef DEBUG
	if (dbg_file.is_open()) {
		dbg_file << ">>>> SetItems: size= " << listItems_.size() << ", winH = " << win_height_ << std::endl;
	}
#endif	
	
}

#define max(a,b) a > b ? a : b;
#define min(a,b) a < b ? a : b;
void ListView::Refresh()
{
#ifdef DEBUG
	if (dbg_file.is_open()) {
		dbg_file << "Entering Refresh: cur=  " << current_index_ << ", start= " <<  start_index_ << ", size= " << listItems_.size() << ", winH = " << win_height_ << std::endl;
	}
#endif

#if 1
	wclear(win_);

	int start_x = 0;
	int disp_size = min (start_index_ + win_height_, listItems_.size());
	for (int i = start_index_; i < disp_size; i++) {
		if (i == current_index_) {
			wattron(win_, A_REVERSE);
			wattron(win_, COLOR_PAIR(color_));
		}
		
		mvwprintw(win_, start_x + 1, 1, listItems_[i].c_str());
		
		if (i == current_index_) {
			wattroff(win_, A_REVERSE);
			wattroff(win_, COLOR_PAIR(color_));
		}
					
		start_x++;
	}

	box(win_, ACS_VLINE, ACS_HLINE);
#else
	for (int i = 0; i < listItems_.size(); i++) {
	
	
		if (i == current_index_) 
			wattron(win_, A_REVERSE);
			
		mvwprintw(win_, i + 1, 1, listItems_[i].c_str());
		
		if (i == current_index_) 
			wattroff(win_, A_REVERSE);		
		
	}
#endif
	wrefresh(win_);

#ifdef DEBUG
	if (dbg_file.is_open()) {
		dbg_file << "Leaving Refresh: cur=  " << current_index_ << ", start= " <<  start_index_ << ", size= " << listItems_.size() << ", winH = " << win_height_ << std::endl;
	}
#endif
}

void ListView::ScrollDown()
{
	current_index_ ++;
	if (current_index_ > listItems_.size() -1)
		current_index_ = listItems_.size() -1;
		
	if (current_index_ > win_height_)
		start_index_ = current_index_ - win_height_;
}

void ListView::ScrollUp()
{
	current_index_--;
	if (current_index_ < 0)
		current_index_ = 0;
		
	if ((start_index_ > 0) && (current_index_ < win_height_))
		start_index_  = 0;
}

void ListView::CursorDown()
{
	if (!focused_)
		return;

#ifdef DEBUG
	if (dbg_file.is_open()) {
		dbg_file << "Entering CursorDown: cur=  " << current_index_ << ", start= " <<  start_index_ << ", size= " << listItems_.size() << ", winH = " << win_height_ << std::endl;
	}
#endif

	current_index_ ++;
	if (current_index_ > listItems_.size() - 1)
		current_index_ = listItems_.size() - 1;
		
	if (current_index_ + 1 >= win_height_)
		start_index_ = current_index_ - win_height_ + 1;
		
#ifdef DEBUG
	if (dbg_file.is_open()) {
		dbg_file << "Leaving CursorDown: cur=  " << current_index_ << ", start= " <<  start_index_ << ", size= " << listItems_.size() << ", winH = " << win_height_ << std::endl;
	}
#endif	
}

void ListView::CursorUp()
{
	if (! focused_)
		return;

#ifdef DEBUG
	if (dbg_file.is_open()) {
		dbg_file << "Entering CursorUp: cur=  " << current_index_ << ", start= " <<  start_index_ << ", size= " << listItems_.size() << ", winH = " << win_height_ << std::endl;
	}
#endif

	current_index_--;
	if (current_index_ < 0)
		current_index_ = 0;

	if ((start_index_ > 0) && (current_index_ < win_height_))
		start_index_ --;

#ifdef DEBUG
	if (dbg_file.is_open()) {
		dbg_file << "Leaving CursorUp: cur=  " << current_index_ << ", start= " <<  start_index_ << ", size= " << listItems_.size() << ", winH = " << win_height_ << std::endl;
	}
#endif
}

void ListView::Select()
{
}


void ListView::SetFocus(bool enableFocus)
{
#ifdef DEBUG
	if (dbg_file.is_open()) {
		dbg_file << "Entering SetFocus: focused_=  " << focused_ << std::endl;
	}
#endif

	focused_ = enableFocus;
	color_ = focused_ ? colors_.focused : colors_.unfocused;
	
#ifdef DEBUG
	if (dbg_file.is_open()) {
		dbg_file << "Leaving SetFocus: focused_=  " << focused_ << std::endl;
	}
#endif	
}

void ListView::ResetCursor()
{
	current_index_ = 0;
	start_index_ = 0;
}
