#include <map>
#include <vector>
#include <string>
using namespace std;

#include <netinet/ip.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "binseq.h"
#include "route.h"

map<BinSeq, vector<BinSeq> > conns;

#define ONEHEX(x) \
{ \
    out += ('A' + x); \
}
string outHex(const BinSeq &in) {
    string out;
    
    for (int i = 0; i < in.size() && i < 10; i++) {
        short hi = ((unsigned char) in[i]) >> 4;
        short lo = ((unsigned char) in[i]) & '\x0F';
        ONEHEX(hi);
        ONEHEX(lo);
    }
    
    return out;
}

void outgvis();

void sigusr1(int ignore)
{
    outgvis();
}

int main()
{
    int fd = socket(PF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in myaddr;
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(3447);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr));
    
    char buf[1025];
    buf[1024] = '\0';
    int rd;
    
    signal(SIGUSR1, sigusr1);
    
    while (rd = recv(fd, (void *) buf, 1024, 0)) {
        if (rd == -1) exit(1);
        
        Route info(BinSeq(buf, rd));
        
        printf("Receieved %s.\n", outHex(info[0]).c_str());
        
        // remove any old data
        if (conns.find(info[0]) != conns.end()) {
            conns.erase(info[0]);
        }
        
        // then add this new data
        for (int i = 1; i < info.size(); i++) {
            conns[info[0]].push_back(info[i]);
        }
    }
}

void outgvis()
{
    // turn our connection info into a graphviz graph
    FILE *gvo = fopen("dht.dot", "w");
    if (!gvo) return;
    
    fputs("digraph {\n  edge [color=red];\n", gvo);
    
    map<BinSeq, vector<BinSeq> >::iterator ci;
    for (ci = conns.begin(); ci != conns.end(); ci++) {
        if (ci->second.size() > 1) {
            fprintf(gvo, "  %s -> %s;\n", outHex(ci->first).c_str(), outHex(ci->second[0]).c_str());
        }
    }
    
    fputs("}\n", gvo);
    
    fclose(gvo);
    
    // now circo it
    system("circo dht.dot -o dht2.dot");
    
    // then add the new data
    FILE *gvi = fopen("dht2.dot", "r");
    gvo = fopen("dht3.dot", "w");
    
    if (!gvi || !gvo) return;
    
    char buf[1025];
    buf[1024] = '\0';
    
    while (!feof(gvi) && !ferror(gvi)) {
        if (fgets(buf, 1024, gvi) <= 0) continue;
        
        if (!strcmp(buf, "}\n")) {
            // add our other info
            map<BinSeq, vector<BinSeq> >::iterator ci;
            for (ci = conns.begin(); ci != conns.end(); ci++) {
                for (int i = 1; i < ci->second.size(); i++) {
                    fprintf(gvo, "  %s -> %s [color=black];\n", outHex(ci->first).c_str(), outHex(ci->second[i]).c_str());
                }
            }
        }
        
        fputs(buf, gvo);
    }
    
    fclose(gvi);
    fclose(gvo);
    
    // and make the proper output
    system("neato -n2 -s dht3.dot -Tps -o dht.ps");
}
