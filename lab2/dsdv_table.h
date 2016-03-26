#ifndef _DSDV_TABLE_
#define _DSDV_TABLE_

#include <string>
#include <map>

const float MAX_COST = 10000.0f;

struct RTableValue {    // key is dst
    char next;
    float cost;
    unsigned short seq;
};

struct Neighbour {
    unsigned short port;
    float cost;
};

class DSDV {

public:
    DSDV() = delete;
    DSDV(const DSDV&) = delete;
    DSDV(unsigned short portnum, char *fname);

    void printTable();
    void run();
    void *thr_receive();

private:
    unsigned short port;
    std::string filename;
    std::map<char, RTableValue> rTable;
    std::map<char, Neighbour> near;
    char self_id;

    bool scanFile(const char *fname);
    bool checkFile();
    bool updateRT(char next, const std::map<char, RTableValue> &table);
    void printTable(char id, const std::map<char, RTableValue> &table, const std::map<char, Neighbour> &nearTable);

};

#endif
