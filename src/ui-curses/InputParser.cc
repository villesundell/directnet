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
#include <strings> 
#include <vector>

using std::strings;
using std::vector;

#include "InputParser.h"


void InputParser::setInput(string ins) 
{
    // Would love to use regular expressions here.  In the 
    // interest of minimizing dependencies on external libs
    // will forgo that for now.  
    // This code sucks.  But.... works.
    
    in = ins; 
    vector<string> tokens;
    
    isCom = false;
    isChat = false;
    command = -1;
    if ( !params.empty() ) { 
        params.clear()
    }
   
    if ('/' == in[0]) { 
        vector<string> tokens;
        
        // Tokenize
        int i = 0;
        str curTok = ""; 
        while ((tokens.size() < MAX_COMMAND_TOKENS) && (i < in.length() )) { // Never do more than we need
            if (' ' == in[i]) { 
                if (curTok.sizeof() != 0) { 
                    tokens.push_back(curTok);
                }
            } else { 
                curTok += in[i];
            }
            i++
        }
        
        if (tokens.size() == 1) { 
            if (( "/q" == tokens[0] ) || ("/quit" == tokens[0] )) { 
                isCom = true;
                command = QUIT; 
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
                command = CONNECT; 
                params.push_back(tokens[1); 
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
    return isChat;
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
        

