#include <iostream>
#include <fstream>

#include "dsdv_table.h"

using namespace std;

DSDV::DSDV(unsigned short portnum, char *fname) 
    : port(portnum), filename(fname) {
   scanFile(fname);
   ports[self_id] = port;
}

void DSDV::printTable() {
    printTable(self_id, rTable, ports);
}

bool DSDV::scanFile(const char *fname) {
    ifstream ifs(fname);
    if(!ifs) return false;  // open error
    int n;      // number of neighbours
    ifs >> n >> self_id;
    for(int i = 0; i < n; i++) {
       char dst;
       float distance;
       unsigned short port;
       ifs >> dst >> distance >> port;
       rTable[dst] = RTableValue{ dst, distance, 0 };
       ports[dst] = port;
    }
    ifs.close();
    return true;
}

void DSDV::printTable(char id, map<char, RTableValue> table, map<char, unsigned short>p) {
    cout << id << endl;
    cout << "dst\tnext\tdstnc\tseq" << endl; 
    for(auto i = table.begin(); i != table.end(); i++) {
        cout << i->first << '\t' 
            << i->second.next << '\t'
            << i->second.distance << '\t'
            << i->second.seq << endl;
    }
    cout << endl;
    cout << "node\tport" << endl;
    for(auto i = p.begin(); i != p.end(); i++) {
        cout << i->first << '\t'
             << i->second << endl;
    }
}
