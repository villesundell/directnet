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

#define CONNECT 0
#define TALK    1
#define ROUTE   2
#define CHAT    3
#define QUIT    4

// Max number of params for longest command
#define MAX_COMMAND_TOKENS
class InputParser { 
    string in = ""; 
    vector<string> params; 
    bool isCom = false; 
    bool isChat = false;
    int command = 0; 
    
    public: 
    bool isCommand(); 
    bool isChat();
    int  getCommand();  
    void setInput(string ins); 
    string getParam(int which); 
}; 


#endif DN_INPUTPARSER_H
