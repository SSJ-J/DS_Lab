#include "dsdv_table.h"
#include "dsdv_transfer.h"
#include <iostream>
#include <fstream>

#include <arpa/inet.h>
#include <netinet/in.h>
using namespace std;

int main(int argc, char *argv[]) {
    if(argc != 3) {
        cout << "usage: " << argv[0] << " <port_num> <file_name>" << endl;
        return 0;
    }
    if(!ifstream(argv[2])) {
        cerr << "Cannot open the file '" << argv[1] << "'" << endl;
        return -1;
    }

    DSDV dsdv(strtol(argv[1], NULL, 10), argv[2]);
    dsdv.printTable();
    dsdv.run();

}
