/*
 * Copyright 2005 Robert Nesius
 *
 * This file is part of DirectNet.
 *
 *    DirectNet is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DirectNet is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DirectNet; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef DN_INPUTPARSER_H
#define DN_INPUTPARSER_H

const int CONNECT      = 0;
const int TALK         = 1;
const int FORCEROUTE   = 2;
const int CHAT         = 3;
const int QUIT         = 4;
const int AWAY         = 5;

// Max number of params for longest command
const int MAX_COMMAND_TOKENS = 2;
class InputParser { 
    string in; 
    vector<string> params; 
    bool isCom; 
    bool chat;
    int command; 
    
    public: 
    InputParser();
    bool isCommand(); 
    bool isChat();
    int  getCommand();  
    void setInput(string ins); 
    string getInput(); // Return the input being processed now
    string getParam(int which); 
}; 


#endif DN_INPUTPARSER_H
