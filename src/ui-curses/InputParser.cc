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
#ifdef DEBUG
extern "C" {
#if defined(USE_NCURSES) && !defined(RENAMED_NCURSES)
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include <unistd.h>
}
#endif

#include <string> 
#include <vector>

using std::string;
using std::vector;

#include "InputParser.h"

#ifdef DEBUG
#include "DisplayWindow.h" 
extern DisplayWindow *displayWin; 
#endif

InputParser::InputParser() 
{ 
    in = ""; 
    isCom = false;
    chat = false; 
    command = 0; 
}

void InputParser::setInput(string ins) 
{
    // Would love to use regular expressions here.  In the 
    // interest of minimizing dependencies on external libs
    // will forgo that for now.  
    // This code sucks.  But.... works.
#ifdef DEBUG
    string debug = "entering setInput"; 
    displayWin->displayMsg(debug);
#endif
    
    in = ins; 
    vector<string> tokens;
    
    isCom = false;
    chat = false;
    command = -1;
    
    if ( !params.empty() ) { 
        params.clear();
    }
#ifdef DEBUG
    debug = "debug: parsing: " + in; 
    displayWin->displayMsg(debug);
#endif
    if ('/' == in[0]) { 
#ifdef DEBUG
        debug = "debug: possible command..."; 
        displayWin->displayMsg(debug);
#endif
        vector<string> tokens;
        // Tokenize
        int i = 0;
        string curTok = ""; 
        while ((tokens.size() < MAX_COMMAND_TOKENS) && (i < in.length() )) { // Never do more than we need
            if (' ' == in[i]) { 
                if (curTok.length() != 0) { 
                    tokens.push_back(curTok);
                    curTok=""; 
                }
            } else { 
                curTok += in[i];
            }
            i++;
        }
        // If we fell out of the loop at the end of the buffer, we have one to flush.
        if (curTok.length() > 0) { 
            tokens.push_back(curTok); 
        }
        
        if (tokens.size() == 1) { 
            if (( "/q" == tokens[0] ) || ("/quit" == tokens[0] )) { 
                isCom = true;
                command = QUIT; 
            }
            if (( "/a" == tokens[0] ) || ("/away" == tokens[0] )) { // clearing
                isCom = true;
                command = AWAY; 
            }
        } else if (tokens.size() == 2) { 
            if (( "/c" == tokens[0] ) || ( "/connect" == tokens[0] )) { 
                isCom = true;
                command = CONNECT; 
                params.push_back(tokens[1]); 
            }
            if (( "/t" == tokens[0] ) || ( "/talk" == tokens[0] )) { 
                isCom = true;
                command = TALK; 
                params.push_back(tokens[1]); 
            }
            if (( "/f" == tokens[0] ) || ( "/force" == tokens[0] )) { 
                isCom = true;
                command = FORCEROUTE; 
                params.push_back(tokens[1]); 
            }
            if (( "/a" == tokens[0] ) || ("/away" == tokens[0] )) { // setting
                isCom = true;
                command = AWAY;
                ins.erase(0, (tokens[0].length() + 1)); 
                params.push_back(ins); 
            }  
            // handle chat here               
        }
    }
    
}

bool InputParser::isCommand() 
{   
    return isCom;
}

bool InputParser::isChat() 
{   
    return chat;
}

int InputParser::getCommand() 
{   
    return command;
}

string InputParser::getParam(int n) { 
    if ( n <= params.size() ) { 
        return ( params[n-1] ); 
    }
}

string InputParser::getInput() { 
    return (in); 
}
        

