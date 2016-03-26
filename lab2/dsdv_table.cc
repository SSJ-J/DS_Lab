#include <iostream>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <cstring>
#include <unistd.h>
#include <assert.h>

#include "dsdv_table.h"
#include "dsdv_transfer.h"

// #define NSEQ     // #seq is useless

using namespace std;

const size_t MAX_BUF = 512;
const unsigned int SEND_INTERVEL = 10;

pthread_mutex_t mutex;

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
    rTable[self_id] = RTableValue{ self_id, 0, 0 };
    pthread_mutex_init(&mutex, NULL);
}

void DSDV::printTable() {
    printTable(self_id, rTable, near);
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
        printTable();
        sleep(SEND_INTERVEL);
        /* send routing table */
        char buf[MAX_BUF];
        pthread_mutex_lock(&mutex);
        if(checkFile()) {
            rTable[self_id].seq += 2;           // add #seq
            // printTable();
        }
        int n = map2buf(rTable, buf, self_id);
        pthread_mutex_unlock(&mutex);
        for(auto i = near.begin(); i != near.end(); i++) {
            trans->send(buf, n, i->second.port);
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
       float cost;
       unsigned short port;
       ifs >> dst >> cost >> port;
       rTable[dst] = RTableValue{ dst, cost, 0 };
       near[dst] = Neighbour{ port, cost };
    }
    ifs.close();
    return true;
}

/* return true if the file has been changed */
bool DSDV::checkFile() {
    ifstream ifs(filename.c_str());
    if(!ifs) return true;  // open error
    int n;      // number of neighbours
    bool flag = false;

    ifs >> n >> self_id;
    for(int i = 0; i < n; i++) {
        char dst;
        float cost;
        unsigned short port;
        unsigned short seq;
        ifs >> dst >> cost >> port;
        if(cost != near[dst].cost) {
            flag = true;
            seq = rTable[dst].seq;
            near[dst].cost = cost;
            rTable[dst] = RTableValue{ dst, cost, seq };
        }
    }
    ifs.close();
    return flag;
}

void *DSDV::thr_receive() {
    Transfer *trans = Transfer::getTransfer();
    int sock = trans->open_sock(port);
    char buf[MAX_BUF];
    while(1) {
        int n = trans->receive(sock, buf);
        map<char, RTableValue> table;
        char id = buf2map(table, buf, n);
        pthread_mutex_lock(&mutex);
        if(updateRT(id, table)) {
            // rTable[self_id].seq += 2;           // add #seq 
            // printTable();
        }
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

/* update routing table */
bool DSDV::updateRT(char next, const map<char, RTableValue>& table) {
    bool flag = false;

    // update whose next hop is 'next' in my routing table
    for(auto i = rTable.begin(); i != rTable.end(); i++) {
        if(i->second.next != next) continue;
        i->second.cost = rTable[next].cost + table.find(i->first)->second.cost;
        i->second.seq = table.find(i->first)->second.seq;
        flag = true;
    }

    for(auto i = table.begin(); i != table.end(); i++) {
        float new_dst = rTable[next].cost + i->second.cost; 
        unsigned short new_seq = i->second.seq;
        if(rTable.find(i->first) == rTable.end()) {     // no this destination
            rTable[i->first] = RTableValue { next, new_dst, new_seq };
            flag = true;
        }
        else {
            float dst = rTable[i->first].cost;
            unsigned short seq = rTable[i->first].seq;
            /* the condition of update */
#ifndef NSEQ    
            if(seq < new_seq) {     // 'next' has been update
                new_dst = near[next].cost + i->second.cost;
                rTable[i->first] = RTableValue { next, new_dst, new_seq };  // recovery
                flag = true;
            }

            else if(seq == new_seq && new_dst < dst) 
#else
            if(new_dst < dst) 
#endif
            {
                rTable[i->first] = RTableValue { next, new_dst, new_seq};
                flag = true;
            }
        }
    }
    return flag;
}

/* print usage information */
void DSDV::printTable(char id, const map<char, RTableValue> &table, const map<char, Neighbour> &p) {
    static int count = 1;
    cout << "## print-out number " << count++ << endl;
    for(auto i = table.begin(); i != table.end(); i++) {
        if(i->first == id) continue;
        cout << "shortest path to node " << i->first 
             << " (seq# " << i->second.seq << "): "
             << "the next hop is " << i->second.next
             << "and the cost is " << i->second.cost <<", "
             << id << " -> " << i->first
             << " : " << i->second.cost << endl;
    }
    cout << endl;
    /*
    cout << "node\tport\tcost" << endl;
    for(auto i = p.begin(); i != p.end(); i++) {
        cout << i->first << '\t'
             << i->second.port << '\t'
             << i->second.cost << endl;
    }
    */
}
