// generated by Fast Light User Interface Designer (fluid) version 1.0107

#ifndef ChatWindow_h
#define ChatWindow_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
extern void closeChat(Fl_Double_Window*, void*);
#include <FL/Fl_Input.H>
extern void sendInput(Fl_Input*, void*);
#include <FL/Fl_Button.H>
extern void flBanUnban(Fl_Button*, void*);
extern void flSendAuthKey(Fl_Button*, void*);
extern void flAddRemAutoFind(Fl_Button*, void*);
#include "Fl_Help_View.H"

class ChatWindow {
public:
  Fl_Double_Window* make_window();
  Fl_Double_Window *chatWindow;
  Fl_Input *textIn;
  Fl_Button *bBan;
  Fl_Button *bSndKey;
  Fl_Button *bAutoFind;
  Fl_Help_View *textOut;
};
#endif
