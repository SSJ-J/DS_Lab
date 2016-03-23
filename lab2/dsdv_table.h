#ifndef _DSDV_TABLE_
#define _DSDV_TABLE_

#include <string>
#include <map>

const float MAX_DIST = 10000.0f;

struct RTableValue {    // key is dst
    char next;
    float distance;
    unsigned short seq;
};

class DSDV {

public:
    DSDV() = delete;
    DSDV(const DSDV&) = delete;
    DSDV(unsigned short portnum, char *fname);

    void printTable();

private:
    unsigned short port;
    std::string filename;
    std::map<char, RTableValue> rTable;
    std::map<char, unsigned short>ports;
    char self_id;

    bool scanFile(const char *fname);
    void printTable(char id, std::map<char, RTableValue> table, std::map<char, unsigned short>p); 
};

#endif
