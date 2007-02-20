// generated by Fast Light User Interface Designer (fluid) version 1.0107

#ifndef BuddyWindow_h
#define BuddyWindow_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
extern void mainWinClosed(Fl_Double_Window*, void*);
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Browser.H>
extern void flSelectBuddy(Fl_Browser*, void*);
#include <FL/Fl_Button.H>
extern void talkTo(Fl_Button*, void*);

class BuddyWindow {
public:
  Fl_Double_Window* make_window();
  Fl_Double_Window *buddyWindow;
  Fl_Menu_Bar *mBar;
  Fl_Browser *onlineList;
  Fl_Button *chatButton;
};
#endif
