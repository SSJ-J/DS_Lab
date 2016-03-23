#include <unistd.h>

#include "dsdv_transfer.h"

const size_t MAX_SIZE = 512;

static Transfer *tInstance = NULL;

Transfer *Transfer::getTransfer() {
    if(tInstance == NULL)
        tInstance = new Transfer();
    return tInstance;
}

int Transfer::open_sock(int port) {
    struct sockaddr_in addr;
    int sock;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // UDP 
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return -1;      // socket error

    if((bind(sock, (struct sockaddr *)&addr, sizeof(addr))) < 0)
        return -2;      // bind error

    return sock;
}

void Transfer::close_sock(int sock) {
    close(sock);
}

int Transfer::receive(int sock, char *buf) {
    return recvfrom(sock, buf, MAX_SIZE-1, 0, NULL, NULL);
}

int Transfer::send(const char *buf, size_t len, int port) {
    struct sockaddr_in addr;    // peer addr
    int sock;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // UDP 
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return -1;      // socket error
    
    return sendto(sock, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
}
