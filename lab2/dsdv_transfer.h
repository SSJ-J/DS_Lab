#ifndef _DSDV_TRANSFER_
#define _DSDV_TRANSFER

#include <cstddef>
#include <netinet/in.h>

/* single-instance class */
class Transfer {

public:
    static Transfer *getTransfer();
    int open_sock(int port);        // for 'receive'
    void close_sock(int sock);      // for 'receive'
    int send(const char *buf, size_t len, int port);    // send to local:port
    int receive(int sock, char *buf); 

private:
    Transfer() { };
    Transfer(const Transfer&) = delete;
};

#endif
