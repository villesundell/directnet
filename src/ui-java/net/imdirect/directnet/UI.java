/*
 * Copyright 2004, 2005, 2006  Gregor Richards
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
 *
 *    As a special exception, the copyright holders of this library give you
 *    permission to link this library with independent modules licensed under
 *    the terms of the Apache License, version 2.0 or later, as distributed by
 *    the Apache Software Foundation.
 */

package net.imdirect.directnet;

import net.imdirect.directnet.DirectNet;
import org.ibex.nestedvm.Runtime;

public class UI {
    public static void main(String[] args) {
        final Runtime rt;
        rt = new DirectNet();

        rt.setCallJavaCB(new Runtime.CallJavaCB() {
            public int call(int a, int b, int c, int d) {
                switch (a) {
                    case 1:
                        {
                            try {
                                // ask the user for their nick, then:
                                rt.call("javaSetNick", new Object[]{"JavaUser"});
                            } catch (Runtime.CallException ce) {
                            } catch (Runtime.FaultException fe) {}
                            break;
                        }

                    case 2:
                        {
                            // display a message
                            try {
                                String from = rt.cstring(rt.memRead(b));
                                String msg = rt.cstring(rt.memRead(b+4));
                                String authmsg = rt.cstring(rt.memRead(b+8));
                                int away = rt.memRead(b+12);
                                // STUB
                            } catch (Runtime.ReadFaultException r) {}
                            break;
                        }

                    case 3:
                        {
                            try {
                                // display a chat message
                                String room = rt.cstring(b);
                                String from = rt.cstring(c);
                                String msg = rt.cstring(d);
                                // STUB
                            } catch (Runtime.ReadFaultException r) {}
                            break;
                        }

                    case 4:
                        {
                            try {
                                // connection established
                                String user = rt.cstring(b);
                                // STUB
                            } catch (Runtime.ReadFaultException r) {}
                            break;
                        }

                    case 5:
                        {
                            try {
                                // route established
                                String user = rt.cstring(b);
                                // STUB
                            } catch (Runtime.ReadFaultException r) {}
                            break;
                        }

                    case 6:
                        {
                            try {
                                // connection lost
                                String user = rt.cstring(b);
                                // STUB
                            } catch (Runtime.ReadFaultException r) {}
                            break;
                        }

                    case 7:
                        {
                            try {
                                // route lost
                                String user = rt.cstring(b);
                                // STUB
                            } catch (Runtime.ReadFaultException r) {}
                            break;
                        }

                    case 8:
                        {
                            // ask whether to import auth key
                            // STUB
                            break;
                        }

                    case 9:
                        {
                            try {
                                // no route to user
                                String user = rt.cstring(b);
                                // STUB
                            } catch (Runtime.ReadFaultException r) {}
                            break;
                        }
                }

                return 0;
            }
        });

        rt.run("directnet", args);
    }
}

