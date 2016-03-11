/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_receiver.h"

#define MAX_SEQ     127
#define WIN_SIZE    10
#define ACK_MAGIC   729

message *rcv_win[WIN_SIZE] = { 0 };
size_t  rcv_win_tailer   = 0;
size_t  n_rcv_win        = 0;  // elements in window
size_t  expected_ack     = 0;

/* 16 bit in Internet checksum */
static inline unsigned short
checksum(unsigned char *addr, size_t count);

/* compare 3 numbers in a ring, b in [a, c] */
static inline bool
between(size_t a, size_t b, size_t c) {
    return ((a <= b && b <= c) || (c<a&&a<=b) || (b<=c&&c<a));
}

static inline void
send_ack(size_t seq) {
    packet *ack = new packet;
    *((unsigned short *)&ack->data[0]) = ACK_MAGIC;
    ack->data[2] = seq;     // no payload
    ack->data[3] = seq;
    ack->data[4] = seq;
    ack->data[5] = seq;
    ack->data[6] = seq;
    Receiver_ToLowerLayer(ack);
    delete ack;
    // printf("send_ack: %lu\n", seq);
}

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    int header_size = 4;    // checksum(2B) + size(1B) + seq(1B)
    size_t rcv_seq  = 0;
    unsigned short cksum = 0;

    /* construct a message and deliver to the upper layer */
    struct message *msg = (struct message*) malloc(sizeof(struct message));
    ASSERT(msg!=NULL);

    msg->size = pkt->data[2];
    rcv_seq = pkt->data[3];
    cksum = *((unsigned short *)pkt->data);

    /* sanity check in case the packet is corrupted */
    if (msg->size<0) msg->size=0;
    if (msg->size>RDT_PKTSIZE-header_size) msg->size=RDT_PKTSIZE-header_size;
    if(cksum != checksum((unsigned char *)&pkt->data[2], msg->size+2)) {
        // printf("DROP!\n");
        return;     // just drop the bad packet
    }

    if(!between(expected_ack, rcv_seq, (expected_ack+WIN_SIZE-1)%(MAX_SEQ+1))) {
        send_ack((expected_ack+MAX_SEQ) % (MAX_SEQ+1));
        return;
    }

    msg->data = (char*) malloc(msg->size);
    ASSERT(msg->data!=NULL);
    memcpy(msg->data, pkt->data+header_size, msg->size);

    /* fill it in a proper position */
    size_t pos = (rcv_seq+MAX_SEQ+1-expected_ack) % (MAX_SEQ+1);    // offset
    pos = (pos+rcv_win_tailer) % WIN_SIZE;
    rcv_win[pos] = msg;
    n_rcv_win++;

    // printf("rcv_seq=%d, e_ack=%d\n", rcv_seq, expected_ack);
    if(rcv_seq == expected_ack) {
        while(n_rcv_win > 0 && rcv_win[rcv_win_tailer] != NULL) {
            msg = rcv_win[rcv_win_tailer];
            Receiver_ToUpperLayer(msg);
            if (msg->data!=NULL) free(msg->data);
            if (msg!=NULL) free(msg);
            rcv_win[rcv_win_tailer] = NULL;
            expected_ack = (expected_ack+1) % (MAX_SEQ+1);
            rcv_win_tailer = (rcv_win_tailer+1) % (WIN_SIZE);
            n_rcv_win--;
            rcv_seq++;
        }

        rcv_seq = (rcv_seq-1) % (MAX_SEQ+1);
        /* send ack */
        send_ack(rcv_seq);    
    } else {
        send_ack((expected_ack+MAX_SEQ) % (MAX_SEQ+1));
    }
}

static inline unsigned short
checksum(unsigned char *addr, size_t count) {
    unsigned int sum = 0;
    unsigned short *sp = (unsigned short *)addr;
    for( ; count > 1; count -= 2) 
        sum += *(sp++);
    if(count > 0)       // count is a odd number
        sum += addr[count - 1];
    // add high 16-bit to low 16-bit
    sum = (sum & 0xFFFF) + (sum >> 16);
    sum = (~sum) & 0xFFFF;
    return sum;
}
