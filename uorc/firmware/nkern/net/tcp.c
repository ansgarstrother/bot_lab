#include <string.h>
#include <stdlib.h>

#include <nkern.h>
#include <net.h>

/***************************************************
BUGS/TODO list

 * Nagle algorithm not implemented (this is a feature...)

 * No persist timer

 * We ignore URG.

 * Special case TIME_WAIT so that we don't tie up a whole tcp_connection_t.
***************************************************/

// approximate memory usage = 2 * WINDOW_SIZE * MAX_TCP_CONNECTIONS
#define MAX_TCP_CONNECTIONS 4

// BSD suggests 200ms delayed ACKs, so we should be reasonably longer than this.
#define TIMEOUT_USECS_INITIAL_THRESHOLD 300000
#define TIMEOUT_USECS_MAX_THRESHOLD (30*1000000)

// How long do we wait around to resend our final ACK when closing
// down a connection? If our ACK is lost, linux will retransmit again
// after 6, 12, 24 seconds. 
#define TCP_TIME_WAIT_TICKS 400

// window size MUST be a power of two.
#define WINDOW_SIZE 512
#define LISTENER_QUEUE_SIZE 4

#define FIN 0x01
#define SYN 0x02
#define RST 0x04
#define PSH 0x08
#define ACK 0x10
#define URG 0x20

//////////////////////////////////////////////////////////////////////////
// KEEP TCP_STATE ENUM AND NAMES IN SYNC
enum tcp_state { TCP_CLOSED = 0, TCP_SYN_SENT = 1, TCP_SYN_RCVD = 2, TCP_ESTABLISHED = 3, 
                 TCP_CLOSE_WAIT = 4, TCP_LAST_ACK = 5, TCP_FIN_WAIT_1 = 6, TCP_FIN_WAIT_2 = 7, 
                 TCP_CLOSING = 8, TCP_TIME_WAIT = 9, TCP_INITIALIZED = 10 };

const char *TCP_STATE_NAMES[] = { "TCP_CLOSED", "TCP_SYN_SENT", "TCP_SYN_RCVD", "TCP_ESTABLISHED", 
                                 "TCP_CLOSE_WAIT", "TCP_LAST_ACK", "TCP_FIN_WAIT_1", "TCP_FIN_WAIT_2", 
                                 "TCP_CLOSING", "TCP_TIME_WAIT", "TCP_INTIALIZED"};
//////////////////////////////////////////////////////////////////////////

// Used to protect global TCP state, such as the listener list.
static nkern_mutex_t mutex;

// when creating outgoing tcp connections, which port do we use?  XXX:
// BUG we assume we can freely pick any high numbered port, heedless
// of other activity on this system.
static uint32_t transient_port;

// A linked list of tcp sockets in "listen" state
static tcp_listener_t *tcplistener_head;

// When we receive an incoming SYN on a port that we are listening to,
// we store the essential information for the SYN in a compact
// structure (rather than allocating a precious TCP connection). A
// call to tcp_accept() will find a pending syn and try to respond to
// it. (This is different than standard BSD sockets, which typically
// fully open incoming connections: on BSD, accept() returns an
// already-connected socket.  Of course, this means that if incoming
// connections are not accepted() in a timely fashion, that the remote
// end might become impatient.
typedef struct tcp_pending_syn tcp_pending_syn_t;
struct tcp_pending_syn
{
    uint32_t    seq;
    ip4_addr_t  remote_addr;
    uint16_t    remote_port;
    uint32_t    win_size;
    uint64_t    utime;      // if zero, this is an invalid syn.
};

// The current implementation has an actual TIME_WAIT state, which
// uses up precious tcp_connection_t structs doing nothing useful. In
// the future, let's keep a separate set of records for connections in
// TIME_WAIT state: they'll need much less state since they only ever
// need to send an ACK.
/*
typedef struct tcp_timewait tcp_timewait_t;
struct tcp_timewait
{
    // XXX TODO
};
*/

struct tcp_listener
{
    int port;

    tcp_pending_syn_t syns[LISTENER_QUEUE_SIZE];

    tcp_listener_t *next;

    nkern_mutex_t mutex;
    nkern_wait_list_t  waitlist;

    int syn_count; // # of syns received
};

static int tx_packet_count; // how many transmitted packets?
static int rx_packet_count; // how many packets total?
static int rx_orphan_packet_count; // how many received packets that had no listener?
static int rx_checksum_err; // how many checksum errors?

// transmit buffer diagram:

// <------> acknowledged by remote host
//        <--------------> transmitted but not yet acked
//                         <----------------------> queued by app, not yet transmitted
// -----------------------------------------------------------------------
// .......| |                   | |                                 | |..... space........
// -----------------------------------------------------------------------
//         ^ first unacked byte  ^ first unsent byte                 ^ first unused byte
//           tx_unacked_pos        tx_unacked_pos + tx_num_unacked     tx_unacked_pos + tx_num_unacked + tx_num_unsent    <-- [0, WINDOW_SIZE - 1]
//           remote_ack                                                                                                   <-- [0, 2^32 -1]
//
// Sequence numbers for transmitted SYNs and FINs are "inserted" into
// sequence space by incrementing remote_ack
struct tcp_connection
{
    enum tcp_state state;

    int idx;                   // used for debugging purposes

    iop_t  iop; 

    ip4_addr_t remote_addr;
    uint16_t remote_port;
    uint16_t local_port;

    uint32_t initial_seq;      // the sequence number corresponding to our SYN

    uint32_t ack;              // bytes we've received. (corresponds to remote_seq)
    uint32_t sent_ack;         // the last ack we sent. (different from 'ack' due to delayed ack)

    uint32_t received_fin;     // have we received a FIN from the remote host? (redundant corresponding TCP states)
    uint32_t sent_fin;         // have we sent a FIN to the remote host? (redundant with TCP states)

    uint8_t  rx[WINDOW_SIZE];  // receive window
    uint32_t rx_inpos;         // next byte to be received by TCP stack [0, WINDOW_SIZE - 1]
    uint32_t rx_outpos;        // next byte to be returned to application [0, WINDOW_SIZE - 1]
    uint32_t rx_space;         // how many bytes can we receive [0, WINDOW_SIZE]
    nkern_wait_list_t rx_waitlist;

    uint8_t  tx[WINDOW_SIZE];  // transmit window
    uint32_t remote_ack;       // highest ack received by host [0, 2^32 - 1]
    uint64_t last_alive_utime;   // utime of last sent or received packet
    uint32_t tx_unacked_pos;   // index into tx buffer corresponding to first unacked byte, [0, WINDOW_SIZE - 1]
    uint32_t tx_num_unacked;   // how many bytes sent but not yet acked? [0, WINDOW_SIZE]
    uint32_t tx_num_unsent;    // how many bytes ready to be sent?  [0, WINDOW_SIZE]
    uint32_t tx_space;         // how many bytes unused in buffer?
    nkern_wait_list_t tx_waitlist;

    uint32_t remote_max_seq;   // what is the maximum sequence number
                               // we can transmit? This is computed
                               // based on the advertised window size
                               // and what they've ACKed.

    uint32_t timeout_usecs_threshold;
    uint32_t timed_wait_ticks; // how many ticks have we been in TIMED_WAIT?
    nkern_mutex_t mutex;

    int tx_byte_count;         // # of bytes
    int rx_byte_count;         // # of bytes
};

static tcp_connection_t connections[MAX_TCP_CONNECTIONS];

static void tcp_task(void *args);

// one-time initialization
void net_tcp_init()
{
    nkern_mutex_init(&mutex, "tcp_global");

    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        connections[i].idx = i;
        nkern_mutex_init(&connections[i].mutex, "tcp_con");
        nkern_wait_list_init(&connections[i].rx_waitlist, "tcp_rx");
        nkern_wait_list_init(&connections[i].tx_waitlist, "tcp_tx");
    }

    nkern_task_create("tcp_timer",
                      tcp_task, NULL,
                      NKERN_PRIORITY_LOW, 1024);
}

// Find an existing TCP connection based on its local and remote port
// numbers. This is a dumb linear search through all known
// connections.
static tcp_connection_t *find_connection(ip4_addr_t remote_addr, uint16_t remote_port, uint16_t local_port)
{
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (connections[i].local_port == local_port &&
            connections[i].remote_addr == remote_addr &&
            connections[i].remote_port == remote_port)
            return &connections[i];
    }

    return NULL;
}

iop_t *tcp_get_iop(tcp_connection_t *con)
{
    return &con->iop;
}

static int tcp_write_iop(iop_t *iop, const void *buf, uint32_t len)
{
    tcp_connection_t *con = (tcp_connection_t*) iop->user;

    for (uint32_t i = 0; i < len; i++) {
        int res = tcp_putc(con, ((char*) buf)[i]);
        if (res < 0)
            return -1;
    }

    return len;
}

static int tcp_read_iop(iop_t *iop, void *buf, uint32_t len)
{
    tcp_connection_t *con = (tcp_connection_t*) iop->user;

    int res = tcp_getc(con);
    if (res < 0)
        return res;

    ((char*) buf)[0] = res;

    return 1;
}

static int tcp_flush_iop(iop_t *iop)
{
    tcp_connection_t *con = (tcp_connection_t*) iop->user;
    return tcp_flush(con);
}

// Allocate a new connection, returning NULL if there are none
// available (a very real possibility!).
static tcp_connection_t *create_connection()
{
    nkern_mutex_lock(&mutex);

    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        // look for a closed connection to reuse.
        if (connections[i].state == TCP_CLOSED) {
            tcp_connection_t *con = &connections[i];

            con->iop.user = con;
            con->iop.write = tcp_write_iop;
            con->iop.read = tcp_read_iop;
            con->iop.flush = tcp_flush_iop;

            con->state = TCP_INITIALIZED;
            con->initial_seq = rand();
            con->remote_addr = 0;
            con->remote_port = 0;
            con->local_port = 0;
            con->ack = 0;
            con->sent_ack = 0;
            con->received_fin = 0;
            con->sent_fin = 0;
            con->rx_inpos = 0;
            con->rx_outpos = 0;
            con->rx_space = WINDOW_SIZE;
            con->remote_ack = 0;
            con->last_alive_utime = nkern_utime();
            con->tx_unacked_pos = 0;
            con->tx_num_unacked = 0;
            con->tx_num_unsent = 0;
            con->tx_space = WINDOW_SIZE;
            con->remote_max_seq = 0;
            con->timed_wait_ticks = 0;
            con->timeout_usecs_threshold = TIMEOUT_USECS_INITIAL_THRESHOLD;
            con->tx_byte_count = 0;
            con->rx_byte_count = 0;
            nkern_mutex_unlock(&mutex);
            return con;
        }
    }

    nkern_mutex_unlock(&mutex);
    return NULL;
}

/**
   Allocate a new TCP packet, with a capacity of 'needed_size'.
   
   @param tcpalloc    Storage is allocated by caller, initialized by this function.
   @param con         An existing TCP connection.
   @param offset      The offset into the packet at which data may be written.
   @param needed_size The number of bytes needed.
**/
static int tcp_packet_alloc(tcp_alloc_t *tcpalloc, tcp_connection_t *con, int *offset, int needed_size)
{
    if (ip_packet_alloc(&tcpalloc->ipalloc, con->remote_addr, offset, needed_size + TCP_HEADER_SIZE))
        return -1;

    tcpalloc->tcp_hdr = (struct tcp_header*) &(tcpalloc->ipalloc.p->data)[*offset];

    *offset += TCP_HEADER_SIZE;

    return 0;
}

/** Send a TCP packet that ACKs what we've seen and sends what we can.
    ASSUMES THAT CON->MUTEX IS HELD.
    @param con        The TCP connection
    @param tcp_flags  TCP flags to use.
**/
static int tcp_send(tcp_connection_t *con, int tcp_flags)
{
    tcp_alloc_t tcpalloc;
    int offset;

    if (con->state == TCP_CLOSED)
        return 0;

    // how many bytes do we want to send? (assuming that bytes before
    // tx_sentpos will be okay.)
    uint32_t willsend = con->tx_num_unsent;
    uint32_t willsend_limit = con->remote_max_seq - (con->remote_ack + con->tx_num_unacked); // limit due to remote window size
    if (willsend > willsend_limit)
        willsend = willsend_limit;

    // It seems like sending data with a SYN should be legal, but linux doesn't like it!
    if (tcp_flags & SYN)
        willsend = 0;

    // XXX TODO: bound willsend per maximum packet size and MTU.

    if (tcp_packet_alloc(&tcpalloc, con, &offset, willsend)) {
        return -1;
    }

    int tcp_size = willsend + TCP_HEADER_SIZE;
    ip_header_fill(&tcpalloc.ipalloc, tcp_size, IP_PROTOCOL_TCP);
    struct tcp_header *tcp_hdr = tcpalloc.tcp_hdr;

    // copy the data.
    uint32_t tx_baseidx = con->tx_unacked_pos + con->tx_num_unacked; 
    for (uint32_t i = 0; i < willsend; i++)
        tcpalloc.tcp_hdr->payload[i] = con->tx[(tx_baseidx + i) & (WINDOW_SIZE-1)];

    net_encode_u16(tcp_hdr->src_port, con->local_port);
    net_encode_u16(tcp_hdr->dst_port, con->remote_port);
    net_encode_u32(tcp_hdr->seq, con->remote_ack + con->tx_num_unacked);
    net_encode_u32(tcp_hdr->ack, con->ack);
    net_encode_u16(tcp_hdr->checksum, 0);
    tcp_hdr->hdr_len = ((TCP_HEADER_SIZE/4)<<4); // header (in words) goes in high 4 bits
    tcp_hdr->flags = tcp_flags;
    net_encode_u16(tcp_hdr->win_size, con->rx_space); // XXX: implement silly window syndrome avoidance
    net_encode_u16(tcp_hdr->urgent, 0);

    con->sent_ack = con->ack;
    con->tx_num_unsent -= willsend;
    con->tx_num_unacked += willsend;
    con->tx_byte_count += willsend;

    con->last_alive_utime = nkern_utime();

    net_packet_t *p = tcpalloc.ipalloc.p;

    uint16_t chk = ip_checksum(tcpalloc.tcp_hdr, tcp_size, 0);
    
    // add in checksum on pseudo-header
    struct ip_header *ip_hdr = tcpalloc.ipalloc.ip_hdr;

    chk = ip_checksum(&ip_hdr->src_ip, 8, chk);
    uint8_t ps[4]; ps[0] = 0; ps[1] = ip_hdr->protocol; ps[2] = tcp_size >> 8; ps[3] = tcp_size & 0xff;
    chk = ip_checksum(ps, 4, chk);
    net_encode_u16(tcp_hdr->checksum, chk ^ 0xffff);

    int res = ip_packet_send(&tcpalloc.ipalloc);

    tx_packet_count++;
    return res;
}

/** Called by the network stack whenever a TCP packet arrives. This
 * implements the data-driven aspect of the TCP finite state
 * machine. (TCP Illustrated, p 241.)
 **/
void net_tcp_receive(net_packet_t *p, struct ip_header *ip_hdr, int offset)
{
    struct tcp_header *tcp_hdr = (struct tcp_header*) &p->data[offset];
    int tcp_size = net_decode_u16(ip_hdr->length) - IP_HEADER_SIZE;

    ////////////////////////////////////////////////////////////////////////
    // checksum test
    if (1) {
        uint16_t chk = ip_checksum(tcp_hdr, tcp_size, 0);
        // add in checksum on pseudo-header
        chk = ip_checksum(&ip_hdr->src_ip, 8, chk);
        uint8_t ps[4]; ps[0] = 0; ps[1] = ip_hdr->protocol; ps[2] = tcp_size >> 8; ps[3] = tcp_size & 0xff;
        chk = ip_checksum(ps, 4, chk);
        
        if (chk != 0xffff) {
            rx_checksum_err++;
            goto cleanup;
        }
    }

    rx_packet_count++;

    ////////////////////////////////////////////////////////////////////////
    // retrieve our connection object by matching on the local and remote port.
    uint16_t local_port = net_decode_u16(tcp_hdr->dst_port);
    uint16_t remote_port = net_decode_u16(tcp_hdr->src_port);
    ip4_addr_t remote_addr = net_decode_ip_addr(ip_hdr->src_ip);

    tcp_connection_t *con = find_connection(remote_addr, remote_port, local_port);
    
    ////////////////////////////////////////////////////////////////////////
    // A new incoming connection? If we have a matching listener,
    // create a new pending SYN record. We don't actually open the
    // connection-- we just record that a SYN has been received. The
    // tcp_accept() call will retrieve one of these records and open
    // it.
    if (con == NULL && (tcp_hdr->flags & SYN)) {

        int matched = 0;

        // are we listening on this port?
        for (tcp_listener_t *tcplistener = tcplistener_head; tcplistener != NULL; tcplistener = tcplistener->next) {

            if (tcplistener->port == local_port) {

                nkern_mutex_lock(&tcplistener->mutex);

                // Invalidate any SYNs that are part of the same
                // connection attempt (we don't need them, we've
                // got fresh new SYN!)
                for (int i = 0; i < LISTENER_QUEUE_SIZE; i++) {
                    if (tcplistener->syns[i].remote_addr == remote_addr && tcplistener->syns[i].remote_port == remote_port) {
                        tcplistener->syns[i].utime = 0;
                    }
                }
                
                // Find either an invalid queue entry, or failing
                // that, the oldest pending syn entry. 
                int oldestidx = 0; // our best entry so far.
                for (int i = 1; i < LISTENER_QUEUE_SIZE; i++) {
                    
                    // Is this entry older or invalid?
                    if (tcplistener->syns[i].utime < tcplistener->syns[oldestidx].utime) {
                        oldestidx = i;

                        // if we found an invalid entry, we can exit the loop now.
                        if (tcplistener->syns[i].utime == 0)
                            break;
                    }
                }

                // alright! Fill in the pending syn
                tcplistener->syns[oldestidx].seq = net_decode_u32(tcp_hdr->seq);
                tcplistener->syns[oldestidx].remote_addr = remote_addr;
                tcplistener->syns[oldestidx].remote_port = remote_port;
                tcplistener->syns[oldestidx].win_size = net_decode_u16(tcp_hdr->win_size);
                tcplistener->syns[oldestidx].utime = nkern_utime();

                tcplistener->syn_count++;
                matched = 1;

                // wake up any tcp_accept() calls.
                nkern_wake_all(&tcplistener->waitlist);
                nkern_mutex_unlock(&tcplistener->mutex);
                break;
            }
        }

        if (!matched) {
            // we're not listening on this port.
            // send a RST.
            tcp_connection_t *con = create_connection();
            if (con != NULL) {
                con->ack = net_decode_u32(tcp_hdr->seq) + 1;
                con->remote_ack = con->initial_seq;
                con->remote_addr = remote_addr;
                con->remote_port = remote_port;
                con->local_port = local_port;
                con->remote_max_seq = con->remote_ack; // no more transmitting.
                tcp_send(con, ACK | RST);

                rx_orphan_packet_count++;
                tcp_free(con);
            }
        }

        goto cleanup;
    }

    if (con == NULL) {
        // It's a stale message. Send RST? (no?)
        goto cleanup;
    }

    nkern_mutex_lock(&con->mutex);

    // Handle RST packets
    if (tcp_hdr->flags & RST) {
        // throw away pending data
        con->rx_outpos = con->rx_inpos;
        con->rx_space = WINDOW_SIZE;
        con->state = TCP_CLOSED;
        nkern_wake_all(&con->rx_waitlist);
        nkern_wake_all(&con->tx_waitlist);
        goto cleanup;
    }

    /////////////////////////////////////////////////////////////////////////////
    uint32_t remote_seq = net_decode_u32(tcp_hdr->seq);
    uint32_t remote_ack = net_decode_u32(tcp_hdr->ack);

    // how many total data bytes received? This counts *all* bytes,
    // even potentially already-received bytes.
    uint32_t rxlen = net_decode_u16(ip_hdr->length) - IP_HEADER_SIZE - TCP_HEADER_SIZE;

    // index of first not-yet-acked data byte.
    int32_t rxoffset = con->ack - remote_seq;

    // is there a gap in the seq #'s? I.e., have we lost a packet?
    uint32_t future_seq = (rxoffset < 0);

    // Always respect the ACK flag...
    if (tcp_hdr->flags & ACK) {
        uint32_t remote_ack = net_decode_u32(tcp_hdr->ack);
        int32_t length_acked = (remote_ack - con->remote_ack);

        // never "unack". This could happen if packets arrived out of order.
        if (length_acked >= 0) {
            con->remote_ack += length_acked;
            con->tx_unacked_pos += length_acked;
            con->tx_num_unacked -= length_acked;
            con->tx_space += length_acked;
            con->timeout_usecs_threshold = TIMEOUT_USECS_INITIAL_THRESHOLD;

            if (con->tx_space > 0)
                nkern_wake_all(&con->tx_waitlist);
        }

        con->last_alive_utime = nkern_utime();
    }

    // Set a flag if we received a fin. Note that we don't respect the
    // FIN flag until we've processed all the data that led up to it.
    uint32_t received_fin = (!future_seq && (tcp_hdr->flags & FIN));
    if (received_fin) {
        con->ack = remote_seq + rxlen + 1;
        con->received_fin = 1;
    }

    // have they acked everything we've sent?
    uint32_t all_acked = (con->tx_num_unacked == 0);

    // update window: they can handle win_size bytes beyond what they've already acknowledged.
    con->remote_max_seq = remote_ack + net_decode_u16(tcp_hdr->win_size);

    ///////////////////////////////////////////////////////////////////////
    // receive the not-yet-received bytes. 

    // Note that where we've received a duplicate transmission with no
    // new data, rxoffset < 0.
    if (rxoffset >= 0 && ((uint32_t) rxoffset) <= rxlen) {

        // trim rxlen so that we won't receive more bytes than we have space in the buffer.
        uint32_t bytes_to_receive = rxlen - rxoffset;
        if (bytes_to_receive > con->rx_space)
            bytes_to_receive = con->rx_space;

        for (uint32_t i = rxoffset; i < rxoffset + bytes_to_receive; i++) {
            con->rx[con->rx_inpos] = tcp_hdr->payload[i];
            con->rx_inpos = (con->rx_inpos + 1) & (WINDOW_SIZE - 1);
            con->rx_space--;
            con->ack++;
        }

        con->rx_byte_count += bytes_to_receive;
    }

    // message lost. Resend.
    if (future_seq) {
        tcp_send(con, ACK);
    }
    
    nkern_wake_all(&con->rx_waitlist);

    ///////////////////////////////////////////////////////////////////////
    switch (con->state)
    {
    case TCP_SYN_RCVD: 
    {
        if (tcp_hdr->flags & SYN) {
            // our SYN | ACK was lost, resend.
            // Play with seq counter so that we send out the same sequence # again.
            con->remote_ack = con->initial_seq;
            tcp_send(con, SYN | ACK);
            con->remote_ack++; 
            break;
        }

        if (!(tcp_hdr->flags & ACK))
            goto cleanup;
        
        con->state = TCP_ESTABLISHED;
        break;
    }

    case TCP_SYN_SENT:
    {
        if ((tcp_hdr->flags & SYN) && (tcp_hdr->flags & ACK)) {
            tcp_send(con, ACK);
            con->state = TCP_ESTABLISHED;
        }

        // simultaneous open
        if (tcp_hdr->flags & SYN) {
            con->remote_ack = con->initial_seq;
            tcp_send(con, SYN | ACK);
            con->remote_ack++;
            break;
        }

        break;
    }

    case TCP_ESTABLISHED:
    {
        if (received_fin) {
            tcp_send(con, ACK);
            con->state = TCP_CLOSE_WAIT;
        }

        break;
    }

    case TCP_CLOSE_WAIT:
        // nothing to do: we wait for application to close()
        break;

    case TCP_LAST_ACK:
    {
        // We've both sent FINs, and we're waiting for our FIN to be acked.

        // did they ack our FIN?
        if (all_acked) {
            con->state = TCP_CLOSED;
        }
        break;
    }

    case TCP_FIN_WAIT_1:
        if (received_fin && all_acked) {
            tcp_send(con, ACK);
            con->state = TCP_TIME_WAIT;
        } else if (received_fin) {
            tcp_send(con, ACK);
            con->state = TCP_CLOSING;
        } else if (all_acked) {
            con->state = TCP_FIN_WAIT_2;
        }
        // otherwise, badness.
        break;

    case TCP_CLOSING:
        if (all_acked)
            con->state = TCP_TIME_WAIT;
        break;

    case TCP_FIN_WAIT_2:
        if (received_fin) {
            tcp_send(con, ACK);
            con->state = TCP_TIME_WAIT;
        }
        break;

    case TCP_TIME_WAIT:
        // our ACK was lost. Send a new one.
        if (received_fin) {
            tcp_send(con, ACK);
        }
        break;

    default:
        kprintf("TCP: closing tcp connection (%d) in bad state %d\n", con->idx, con->state);
        con->state = TCP_CLOSED;
        break;
    }

    
cleanup:
    if (con)
        nkern_mutex_unlock(&con->mutex);

    p->free(p);
}

// periodically service tcp tasks (i.e., retransmits)
// linux delayed ack timer: 12us (on my x86_64 system, 2.6.28). (BSD, TCP/IP illustrated: 200ms)
static void tcp_task(void *args)
{
    while(1) {
        nkern_usleep(50000);

        for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
            tcp_connection_t *con = &connections[i];

            nkern_mutex_lock(&con->mutex);

            if (con->state != TCP_CLOSED) {

                if ((nkern_utime() - con->last_alive_utime) > con->timeout_usecs_threshold) {

                    // increase timeout for next time.
                    con->timeout_usecs_threshold *= 2;
                    if (con->timeout_usecs_threshold > TIMEOUT_USECS_MAX_THRESHOLD)
                        con->timeout_usecs_threshold = TIMEOUT_USECS_MAX_THRESHOLD;

                    int is_timeout = 0;

                    switch (con->state)
                    {
                    case TCP_INITIALIZED:
                        assert(0);
                        break;
                    case TCP_SYN_RCVD:
                        con->remote_ack = con->initial_seq;
                        // must also retransmit all data we previously pushed out since the SYN didn't make it.
                        con->tx_num_unsent += con->tx_num_unacked; 
                        con->tx_num_unacked = 0;
                        tcp_send(con, SYN | ACK);
                        con->remote_ack++;
                        is_timeout = 1;
                        break;
                    case TCP_SYN_SENT:
                        con->remote_ack = con->initial_seq;
                        tcp_send(con, SYN);
                        con->remote_ack++;
                        is_timeout = 1;
                        break;          
                    case TCP_ESTABLISHED:
                        if (con->tx_num_unacked + con->tx_num_unsent > 0) {
                            // we want to resend our "sent but not acked" bytes. Do this by 
                            // internally recording those bytes as "not yet sent".
                            kprintf("tcp established retransmit: unsent %d, unacked %d, space %d\n", con->tx_num_unsent, con->tx_num_unacked, con->tx_space);
                            con->tx_num_unsent += con->tx_num_unacked;
                            con->tx_num_unacked = 0;
                            tcp_send(con, ACK); // this will retransmit data too.
                            is_timeout = 1;
                        }
                        break;
                    case TCP_CLOSE_WAIT:
                    case TCP_CLOSING:
                        tcp_send(con, ACK);
                        is_timeout = 1;
                        break;
                    case TCP_FIN_WAIT_2:
                        break;
                    case TCP_LAST_ACK:
                    case TCP_FIN_WAIT_1:
                        con->remote_ack--;
                        tcp_send(con, ACK | FIN);
                        con->remote_ack++;
                        is_timeout = 1;
                        break;
                    case TCP_CLOSED: // silence compiler warning
                        // don't send anything.
                        break;
                    case TCP_TIME_WAIT:
                        break;
                    }

                    if (is_timeout)
                        kprintf("TCP: Retransmission timer triggered for con %d in state %s\n", con->idx, TCP_STATE_NAMES[con->state]);
                }

                if (con->state == TCP_TIME_WAIT) {
                    con->timed_wait_ticks++;
                    if (con->timed_wait_ticks >= TCP_TIME_WAIT_TICKS)
                        con->state = TCP_CLOSED;
                }

                // delayed ACK
                if (con->sent_ack != con->ack) 
                    tcp_send(con, ACK);
            }

            nkern_mutex_unlock(&con->mutex);
        }
    }
}

//////////////////////////////////////////////////////////
// User API functions below

/** Begin listening for connections on the given port.  Returns 0 on
    success.
**/
tcp_listener_t *tcp_listen(int port)
{                                               
    tcp_listener_t *tcplistener = calloc(sizeof(tcp_listener_t),1);
    tcplistener->port = port;

    nkern_mutex_lock(&mutex);

    // make sure no one already listening on this port
    for (tcp_listener_t *tcplistener = tcplistener_head; tcplistener != NULL; tcplistener = tcplistener->next) {
        if (tcplistener->port == port) {
            nkern_mutex_unlock(&mutex);
            return NULL;
        }
    }

    tcplistener->next = tcplistener_head;
    tcplistener_head = tcplistener;
    
    nkern_mutex_unlock(&mutex);

    nkern_mutex_init(&tcplistener->mutex, "tcpm");
    nkern_wait_list_init(&tcplistener->waitlist, "tcp_listen");

    return tcplistener;
}

/** Wait for a connection on the specified port. 
 **/
tcp_connection_t *tcp_accept(tcp_listener_t *tcplistener)
{
    tcp_connection_t *con = NULL;
    int newestidx;

    while (1) {
        // find the most recent pending syn (not the oldest: this
        // increases our likelihood of successfully servicing at least
        // one incoming response. The oldest pending syn could have
        // already timed out.
        uint64_t utime = nkern_utime();

        nkern_mutex_lock(&tcplistener->mutex);

        newestidx = 0;
        for (int i = 1; i < LISTENER_QUEUE_SIZE; i++) {
            if (tcplistener->syns[i].utime > tcplistener->syns[newestidx].utime)
                newestidx = i;
        }
        
        // if we didn't find a reasonably recent SYN, then wait.
        if (tcplistener->syns[newestidx].utime == 0 || utime - tcplistener->syns[newestidx].utime > 15000000ULL) {
            nkern_unlock_and_wait(&tcplistener->mutex, &tcplistener->waitlist);
            continue;
        }

        // mark this one as "tried".
        tcplistener->syns[newestidx].utime = 0;

        // try allocating a TCP structure.
        con = create_connection();

        // if no buffer, wait for the next SYN.
        if (con == NULL) {
            nkern_mutex_unlock(&tcplistener->mutex);
            nkern_usleep(5000); // XXX Hack!
            continue;
        }

        // we're looking good... escape our wait loop.
        break;
    }

    // fill in the connection structure.
    nkern_mutex_lock(&con->mutex);
    con->ack = tcplistener->syns[newestidx].seq + 1;
    con->remote_ack = con->initial_seq;
    con->remote_addr = tcplistener->syns[newestidx].remote_addr;
    con->remote_port = tcplistener->syns[newestidx].remote_port;
    con->local_port = tcplistener->port;
    con->remote_max_seq = con->remote_ack + tcplistener->syns[newestidx].win_size;

    // send a response SYN.
    con->remote_ack = con->initial_seq;
    tcp_send(con, SYN | ACK);
    con->remote_ack++;
    con->state = TCP_SYN_RCVD;
    nkern_mutex_unlock(&con->mutex);

    nkern_mutex_unlock(&tcplistener->mutex);
    return con;
}


tcp_connection_t *tcp_connect(ip4_addr_t host, int port)
{
    tcp_connection_t *con = create_connection();
    if (con == NULL)
        return NULL;

    nkern_mutex_lock(&mutex);

    con->remote_addr = host;
    con->remote_port = port;
    con->local_port = 40000 + (transient_port&0x3ff);
    transient_port++;

    con->remote_ack = con->initial_seq;
    tcp_send(con, SYN);
    con->remote_ack++;

    con->state = TCP_SYN_SENT;
    nkern_mutex_unlock(&mutex);

    return con;
}

/** Blocking wait for a character on the tcp connection. Returns EOF
 * (-1) if the connection has failed or the remote side has closed. 
 **/
int tcp_getc(tcp_connection_t *con)
{
retry:
    nkern_mutex_lock(&con->mutex);

    if (con->rx_space == WINDOW_SIZE) {
        // rx buffer is empty.

        // if the remote side has sent FIN, return -1.
        if (con->state == TCP_CLOSED || con->received_fin) {
            nkern_mutex_unlock(&con->mutex);
            return -1;
        }

        // the connection is still open (as far as we know). Wait for more data.
        nkern_unlock_and_wait(&con->mutex, &con->rx_waitlist);
        goto retry;
    }

    // a character is available!
    char c = con->rx[con->rx_outpos];
    con->rx_outpos = (con->rx_outpos + 1) & (WINDOW_SIZE - 1);
    con->rx_space++;
    
    // If we've just opened up the buffer beyond the 2/3 point, send
    // an ACK. We want to encourage the remote side to go ahead and
    // send more data. Linux won't immediately send additional data
    // unless we report half our window open; Linux will wait for a
    // while, which greatly decreases our throughput.
    if (con->rx_space == (2*WINDOW_SIZE/3))
        tcp_send(con, ACK);

    nkern_mutex_unlock(&con->mutex);

    return c;
}

/** Enqueue a character for transmission. See tcp_flush(). Can block
 * (e.g., if the tx buffer is full). Returns -1 if the connections has
 * failed.
 **/
int tcp_putc(tcp_connection_t *con, char c)
{
retry:
    nkern_mutex_lock(&con->mutex);

    // check for closed connection and return -1.
    if (con->state == TCP_CLOSED) {
        nkern_mutex_unlock(&con->mutex);
        return -1;
    }

    if (con->tx_space == 0) {
        tcp_flush(con); 
        nkern_unlock_and_wait(&con->mutex, &con->tx_waitlist);
        goto retry;
    }

    con->tx[(con->tx_unacked_pos + con->tx_num_unacked + con->tx_num_unsent) & (WINDOW_SIZE - 1)] = c;
    con->tx_num_unsent++;
    con->tx_space--;
    
    nkern_mutex_unlock(&con->mutex);

    return 0;
}

/** Flush the transmit buffer. Characters are not guaranteed to be
    transmitted at all until flush() is called. (This is different
    than POSIX). Returns -1 if the connection has failed. **/
int tcp_flush(tcp_connection_t *con)
{
    nkern_mutex_lock(&con->mutex);
    int res = tcp_send(con, ACK | PSH);
    nkern_mutex_unlock(&con->mutex);

    return res;
}

/** Close the tcp connection for future writing (performing a
 * half-close). Future reads are still allowed, and the resources for
 * the connection are NOT freed.
 **/
void tcp_close(tcp_connection_t *con)
{
    nkern_mutex_lock(&con->mutex);

    if (con->tx_space < WINDOW_SIZE)
        tcp_flush(con);

    // nothing to do?
    if (con->state == TCP_CLOSED || con->sent_fin) {
        nkern_mutex_unlock(&con->mutex);
        return;
    }

    // if connection hasn't finished setting up yet, we must
    // delay. (It's illegal to send a FIN in SYN_RCVD or SYN_SENT.)
    // Also, don't start shutdown until we've transmitted all pending
    // data.
    while (con->state == TCP_SYN_RCVD || con->state == TCP_SYN_SENT || con->tx_space != WINDOW_SIZE) {

        // XXX gross! should arrange for a wake up?
        nkern_mutex_unlock(&con->mutex);
        nkern_usleep(5000);
        nkern_mutex_lock(&con->mutex);
    }
    
    // Send FIN.
    tcp_send(con, ACK | FIN);
    con->remote_ack++; // FIN must be acked
    con->sent_fin = 1;

    if (con->state == TCP_SYN_RCVD || con->state == TCP_ESTABLISHED) {
        con->state = TCP_FIN_WAIT_1;
    } else if (con->state == TCP_CLOSE_WAIT) {
        con->state = TCP_LAST_ACK;
    } else {
        kprintf("TCP: received close() in funny state (%d)\n", con->state);
        con->state = TCP_CLOSED;
    }

    nkern_mutex_unlock(&con->mutex);
}

/** Free resources associated with connection. **/
void tcp_free(tcp_connection_t *con)
{
    // nothing to do. Don't mark the connection as CLOSED, because we
    // might actually be in TCP_LAST_ACK or some other waiting state.

    if (con->state == TCP_INITIALIZED)
        con->state = TCP_CLOSED;
}

void tcp_print_stats(iop_t *iop)
{
    pprintf(iop, "TCP: RX packets %8d, TX packets %8d, checksum err %8d, orphans %8d\n",
            rx_packet_count, tx_packet_count, rx_checksum_err, rx_orphan_packet_count);

    pprintf(iop, "  ---- Listeners ----\n");
    for (tcp_listener_t *tcplistener = tcplistener_head; tcplistener != NULL; tcplistener = tcplistener->next) {
        pprintf(iop, "  local port %-5d, SYN connection attempts %d\n", tcplistener->port, tcplistener->syn_count);
    }

    pprintf(iop, "  ---- Connections ----\n");
    for (int conidx = 0; conidx < MAX_TCP_CONNECTIONS; conidx++) {
        tcp_connection_t *con = &connections[conidx];
        char remote_addr_string[24];
        inet_addr_to_dotted(con->remote_addr, remote_addr_string);
        pprintf(iop, "  [%2d] local:%-5d <--> %17s:%-5d    (%s)\n", con->idx, con->local_port, remote_addr_string, con->remote_port, TCP_STATE_NAMES[con->state]);
        pprintf(iop, "       rx bytes = %8d, tx bytes = %8d\n", con->rx_byte_count, con->tx_byte_count);
    }
}

