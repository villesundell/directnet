// generated by Fast Light User Interface Designer (fluid) version 1.0106

#include "ChatWindow.h"

Fl_Double_Window* ChatWindow::make_window() {
  Fl_Double_Window* w;
  { Fl_Double_Window* o = chatWindow = new Fl_Double_Window(360, 315, "User");
    w = o;
    o->callback((Fl_Callback*)closeChat, (void*)(this));
    { Fl_Input* o = textIn = new Fl_Input(0, 265, 360, 25);
      o->callback((Fl_Callback*)sendInput);
      o->when(FL_WHEN_ENTER_KEY);
    }
    { Fl_Button* o = bSndKey = new Fl_Button(0, 290, 180, 25, "Send Authentication Key");
      o->callback((Fl_Callback*)flSendAuthKey);
    }
    { Fl_Button* o = bAutoFind = new Fl_Button(180, 290, 180, 25, "Add to Autofind List");
      o->callback((Fl_Callback*)flAddRemAutoFind);
    }
    { Fl_Text_Display* o = textOut = new Fl_Text_Display(0, 0, 360, 260);
      Fl_Group::current()->resizable(o);
      textOut->buffer(new Fl_Text_Buffer(32256));
      textOut->wrap_mode(1, 0);
    }
    o->end();
  }
  return w;
}
