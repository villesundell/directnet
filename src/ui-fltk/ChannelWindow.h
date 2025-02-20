// generated by Fast Light User Interface Designer (fluid) version 1.0107

#ifndef ChannelWindow_h
#define ChannelWindow_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
extern void closeChat(Fl_Double_Window*, void*);
#include <FL/Fl_Input.H>
extern void sendInput(Fl_Input*, void*);
#include <FL/Fl_Tile.H>
#include <FL/Fl_Browser.H>
#include "Fl_Help_View.H"

class ChannelWindow {
public:
  Fl_Double_Window* make_window();
  Fl_Double_Window *channelWindow;
  Fl_Input *textIn;
  Fl_Browser *userList;
  Fl_Help_View *textOut;
};
#endif
