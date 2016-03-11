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

packet *win[WIN_SIZE];    // add a space to judge full/empty
packet *buffer[BSIZE];    // add a space to judge full/empty  
unsigned short snd_win_t = 0;   // tailer of window
unsigned short n_snd_win = 0;   // size of window
size_t buf_t    = 0;     // tailer of buffer
size_t n_buf    = 0;     // size of buffer
size_t next_seq = 0;

/* 16 bit in Internet checksum */
static inline unsigned short
checksum(unsigned char *addr, size_t count);

/* compare 3 numbers in a ring, b in [a, c] */
static inline bool
between(size_t a, size_t b, size_t c) {
    return ((a <= b && b <= c) || (c<a&&a<=b) || (b<=c&&c<a));
}

/* operations in the ring mod by 'max' */
static inline size_t
add(size_t a, size_t b, size_t max) {
    return ((a+b) % max);
}

static inline size_t
minus(size_t a, size_t b, size_t max) {
    return ((a+max-b) % max);
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    int header_size = 4;        //  checksum(2B) + size(1B) + seq(1B)

    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - header_size;

    /* split the message if it is too big */
    packet *pkt = NULL;

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;
    for ( ;cursor < msg->size; cursor += maxpayload_size) {
	    /* fill in the packet */
        pkt = new packet;
	    pkt->data[2] = (maxpayload_size < (msg->size - cursor))? 
                        maxpayload_size : (msg->size - cursor);
        pkt->data[3] = next_seq;
        next_seq = (next_seq+1) % (MAX_SEQ+1);      // update seq
        memcpy(pkt->data+header_size, msg->data+cursor, pkt->data[2]);
        *((unsigned short *)&pkt->data[0]) 
                = checksum((unsigned char *)&pkt->data[2], pkt->data[2]+2);
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
        // printf("Upper-seq=%d\n", pkt->data[3]);
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
        return;
    }
    if(n_snd_win == 0) return;

    int ack = pkt->data[3];
    int min_ack = win[snd_win_t]->data[3];
    if(ack!=(min_ack+MAX_SEQ)%(MAX_SEQ+1))
        Sender_StopTimer();
    // printf("receive_ack: %d\n", ack);

    size_t dpos = minus(ack, min_ack, MAX_SEQ+1);
    if(dpos >= n_snd_win) {
        if(n_snd_win != 0 && !Sender_isTimerSet())
            Sender_StartTimer(TIMEOUT);
        return;
    }
        

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
