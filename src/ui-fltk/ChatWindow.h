// generated by Fast Light User Interface Designer (fluid) version 1.0106

#ifndef ChatWindow_h
#define ChatWindow_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
extern void sendInput(Fl_Input*, void*);
#include <FL/Fl_Text_Display.H>

class ChatWindow {
public:
  Fl_Double_Window* make_window();
  Fl_Double_Window *chatWindow;
  Fl_Input *textIn;
  Fl_Text_Display *textOut;
};
#endif
