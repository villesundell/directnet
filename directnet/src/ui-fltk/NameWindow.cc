// generated by Fast Light User Interface Designer (fluid) version 1.0106

#include "NameWindow.h"

Fl_Double_Window* NameWindow::make_window() {
  Fl_Double_Window* w;
  { Fl_Double_Window* o = new Fl_Double_Window(255, 45);
    w = o;
    o->user_data((void*)(this));
    { Fl_Input* o = nameIn = new Fl_Input(0, 20, 255, 25, "  Nickname:");
      o->callback((Fl_Callback*)setName);
      o->align(FL_ALIGN_TOP_LEFT);
      o->when(FL_WHEN_ENTER_KEY);
    }
    o->end();
  }
  return w;
}
