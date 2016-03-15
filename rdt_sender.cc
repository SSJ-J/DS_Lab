/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
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
#include "rdt_sender.h"

#define MAX_SEQ     127
#define WIN_SIZE    10
#define BSIZE       100000
#define TIMEOUT     0.1
#define ACK_MAGIC   729

#define P_CKSUM     0
#define P_SIZE      4
#define P_SEQ       5

packet *win[WIN_SIZE];    // add a space to judge full/empty
packet *buffer[BSIZE];    // add a space to judge full/empty  
unsigned short snd_win_t = 0;   // tailer of window
unsigned short n_snd_win = 0;   // size of window
size_t buf_t    = 0;     // tailer of buffer
size_t n_buf    = 0;     // size of buffer
size_t next_seq = 0;     // next #seq to send

/* 32 bit in Internet checksum */
static inline unsigned int
checksum(unsigned char *addr, size_t count);

/* sender initialization, called once at the very beginning */
void Sender_Init() {
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final() {
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    int header_size = 6;        //  checksum(4B) + size(1B) + seq(1B)

    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - header_size;

    /* split the message if it is too big */
    packet *pkt = NULL;

    /* the cursor always points to the first unsent byte in the message */
    for (int cursor = 0;cursor < msg->size; cursor += maxpayload_size) {
	    /* fill in the packet */
        pkt = new packet;
	    pkt->data[P_SIZE] = (maxpayload_size < (msg->size - cursor))? 
                        maxpayload_size : (msg->size - cursor);
        pkt->data[P_SEQ] = next_seq;
        next_seq = (next_seq+1) % (MAX_SEQ+1);      // update seq
        memcpy(pkt->data+header_size, msg->data+cursor, pkt->data[P_SIZE]);
        *((unsigned int *)&pkt->data[P_CKSUM]) 
                = checksum((unsigned char *)&pkt->data[P_SIZE], pkt->data[P_SIZE]+2);

        /* put it in window or buffer */
	    if(n_snd_win < WIN_SIZE) {  // window is not full
            unsigned short pos = (snd_win_t+n_snd_win) % WIN_SIZE;    
            win[pos] = pkt;
            n_snd_win++;
            /* send it out through the lower layer */
	        Sender_ToLowerLayer(pkt);
            if(!Sender_isTimerSet())
                Sender_StartTimer(TIMEOUT);
        } else if (n_buf < BSIZE) {  // buffer is not full
            size_t pos = (buf_t+n_buf) % BSIZE;
            buffer[pos] = pkt;
            n_buf++;
        } else {
            ASSERT(false);
        }
        // printf("Upper-seq=%d\n", pkt->data[P_SEQ]);
    }
    if(n_snd_win != 0 && !Sender_isTimerSet())
        Sender_StartTimer(TIMEOUT);
}
/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt) {
    packet *p = NULL;

    // receive excepted ack
    unsigned char *cp = (unsigned char *)pkt->data;
    unsigned short magic = *((unsigned short *)&pkt->data[0]);
    if(magic != ACK_MAGIC || cp[2] != cp[3] || cp[3] != cp[4] || cp[4] != cp[5] || cp[5] != cp[6]) {
        //printf("DROP ACK\n");
        return;     // check failed
    }
    if(n_snd_win == 0) return;

    int ack = pkt->data[P_SEQ];
    int min_ack = win[snd_win_t]->data[P_SEQ];
    size_t dpos = (ack+ MAX_SEQ + 1 - min_ack) % (MAX_SEQ + 1);
    if(dpos >= n_snd_win) return;   // no this ack

    if(ack!=(min_ack+MAX_SEQ)%(MAX_SEQ+1))      // min_ack - 1 means send fails
        Sender_StopTimer();
    // printf("receive_ack: %d\n", ack);
        
    for(size_t i=0; i < dpos+1; i++) {
        delete win[snd_win_t++];
        snd_win_t %= WIN_SIZE;
        n_snd_win--;
    }

    while(n_buf != 0 && n_snd_win != WIN_SIZE) {
        // buffer is not empty and window is not full
        p = win[(snd_win_t+n_snd_win)%(WIN_SIZE)] = buffer[buf_t++];
        n_snd_win++;
        n_buf--;
        buf_t %= BSIZE;
        if(!Sender_isTimerSet())
            Sender_StartTimer(TIMEOUT);
        Sender_ToLowerLayer(p);
    }

    if(n_snd_win != 0 && !Sender_isTimerSet())
        Sender_StartTimer(TIMEOUT);
}

/* event handler, called when the timer expires */
void Sender_Timeout() {
    /* resend */
    // printf("Time_out!, seq=%d\n", win[snd_win_t]->data[3]);
    if(n_snd_win != 0) {
        Sender_ToLowerLayer(win[snd_win_t]); 
        Sender_StartTimer(TIMEOUT);
    }
}

static inline unsigned int
checksum(unsigned char *addr, size_t count) {
    unsigned long long int sum = 0;
    unsigned short *sp = (unsigned short *)addr;
    for( ; count > 1; count -= 2) 
        sum += *(sp++);
    if(count > 0)       // count is a odd number
        sum += addr[count - 1];
    // add high 16-bit to low 16-bit
    sum = (sum & 0xFFFFFFFF) + (sum >> 32);
    sum = (~sum) & 0xFFFFFFFF;
    return sum;
}
