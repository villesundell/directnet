// generated by Fast Light User Interface Designer (fluid) version 1.0107

#include "BuddyWindow.h"

Fl_Double_Window* BuddyWindow::make_window() {
  Fl_Double_Window* w;
  { Fl_Double_Window* o = buddyWindow = new Fl_Double_Window(255, 430, "DirectNet");
    w = o;
    o->color(FL_FOREGROUND_COLOR);
    o->callback((Fl_Callback*)mainWinClosed, (void*)(this));
    { Fl_Menu_Bar* o = mBar = new Fl_Menu_Bar(0, 0, 255, 25);
      o->box(FL_FLAT_BOX);
    }
    { Fl_Browser* o = onlineList = new Fl_Browser(0, 25, 255, 355);
      o->type(2);
      o->labelcolor(FL_BACKGROUND2_COLOR);
      o->callback((Fl_Callback*)flSelectBuddy);
      o->align(FL_ALIGN_TOP_LEFT);
      o->when(3);
      Fl_Group::current()->resizable(o);
    }
    { Fl_Button* o = chatButton = new Fl_Button(0, 380, 255, 25);
      o->callback((Fl_Callback*)talkTo);
      o->deactivate();
    }
    { Fl_Output* o = status = new Fl_Output(0, 405, 255, 25);
      o->box(FL_FLAT_BOX);
    }
    o->end();
  }
  return w;
}
