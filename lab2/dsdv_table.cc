#include <iostream>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <cstring>
#include <unistd.h>
#include <assert.h>

#include "dsdv_table.h"
#include "dsdv_transfer.h"

const size_t MAX_BUF = 512;

const unsigned int SEND_INTERVEL = 10;

using namespace std;

// convert map to char[], return its size
static inline int 
map2buf(const map<char, RTableValue> &table, char *buf, char self_id) {
    buf[0] = self_id;
    size_t p = 1;
    for(auto i = table.begin(); i != table.end(); i++) {
        buf[p] = i->first;
        p += sizeof(char);
        *((RTableValue *) &buf[p]) = i->second;
        p += sizeof(RTableValue);
        assert(p <= MAX_BUF);
    }
    return p;
}

// convert char[] of length 'len' to map, return id of this table
static inline char
buf2map(map<char, RTableValue> &table, const char *buf, size_t len) {
    assert(len <= MAX_BUF);
    char id = buf[0];
    for(size_t p = 1; p < len; p += (1+sizeof(RTableValue))) 
        table[buf[p]] = *((RTableValue *) &buf[p+1]);
    return id;
}

DSDV::DSDV(unsigned short portnum, char *fname) 
    : port(portnum), filename(fname) {
    scanFile(fname);
    // ports[self_id] = port;
    rTable[self_id] = RTableValue{ self_id, 0, 0 };
}

void DSDV::printTable() {
    printTable(self_id, rTable, ports);
}

/* helper function used in pthread_create */
void *thr_receive_helper(void *arg) {
    DSDV *dsdv = (DSDV *) arg;
    return dsdv->thr_receive();
}

void DSDV::run() {
    Transfer *trans = Transfer::getTransfer();
    pthread_t rtid;
    int e;

    if((e = pthread_create(&rtid, NULL, thr_receive_helper, this)) != 0) {
        cerr << "DSDV::run error: " << strerror(e) << endl;
    }
    
    while(1) {
        sleep(SEND_INTERVEL);
        /* send routing table */
        char buf[MAX_BUF];
        rTable[self_id].seq += 2;           // add #seq
        int n = map2buf(rTable, buf, self_id);
        for(auto i = ports.begin(); i != ports.end(); i++) {
            trans->send(buf, n, i->second);
        }
    }

    pthread_join(rtid, NULL);
}

/* read the configuration file */
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

/* print usage information */
void DSDV::printTable(char id, const map<char, RTableValue> &table, const map<char, unsigned short> &p) {
    cout << "==========================" << endl;
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

void *DSDV::thr_receive() {
    Transfer *trans = Transfer::getTransfer();
    int sock = trans->open_sock(port);
    char buf[MAX_BUF];
    while(1) {
        int n = trans->receive(sock, buf);
        map<char, RTableValue> table;
        char id = buf2map(table, buf, n);
        updateRT(id, table);
        printTable();
    }

    return NULL;
}

/* update routing table */
void DSDV::updateRT(char next, const map<char, RTableValue>& table) {
    for(auto i = table.begin(); i != table.end(); i++) {
        float new_dst = rTable[next].distance + i->second.distance; 
        unsigned short new_seq = i->second.seq;
        if(rTable.find(i->first) == rTable.end())      // no this destination
            rTable[i->first] = RTableValue { next, new_dst, new_seq };
        else {
            float dst = rTable[i->first].distance;
            unsigned short seq = rTable[i->first].seq;
            /* the condition of update */
            if((seq == new_seq && new_dst < dst) || seq < new_seq) 
                rTable[i->first] = RTableValue { next, new_dst, new_seq};
        }
    }
}
