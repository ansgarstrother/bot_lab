#ifndef _NET_H
#define _NET_H

#include <nkern.h>
#include <stdint.h>

typedef struct net_packet_t  net_packet_t;
typedef struct net_dev_t     net_dev_t;

// Ethernet addresses are represented as they are written:
// 00:0f:66:a0:ca:21 becomes 0x000f66a0ca21. Note that the ethernet
// standard uses network byte order (big endian).

typedef uint64_t eth_addr_t;
typedef uint32_t ip4_addr_t;

///////////////////////////////////////////////////////////////////
/** Devices do their own buffer allocation, which allows them to make
    sure the buffers are suitable for DMA. (On LPC devices, for
    example, there's a separate RAM bank for ethernet packets.) **/
struct net_packet_t
{
    uint8_t       *data;
    int            size;
    int            capacity;

    net_dev_t     *netdev;
    void          (*free)(net_packet_t *p);

    net_packet_t  *next_packet; // can be used by the net_packet_t allocator
};

/** This structure defines an IP interface, bound to a particular
    device. These are basically equivalent to routes. **/
typedef struct ip_config_t ip_config_t;
struct ip_config_t
{
    ip4_addr_t     ip_addr;
    ip4_addr_t     ip_mask;
    ip4_addr_t     gw_ip_addr;

    net_dev_t      *netdev; // back link to our netdev
    ip_config_t    *next;
};

///////////////////////////////////////////////////////////////////
// A network interface need only implement two methods.  All packets
// that will be sent via packet_send will be allocated via
// packet_alloc. This allows the network device to allocate a DMA-able
// buffer and potentially avoid a memcpy. On success, the user will
// write up to needed_size bytes beginning at offset. (This allows the
// network driver to allocate a few bytes at the beginning of the
// buffer for an (e.g.) ethernet header.)
//
// Of course, most network devices need to implement a receive
// pipeline too. This can call (e.g.) net_ethernet_receive.
//
struct net_dev_t
{
    net_packet_t* (*packet_alloc)(net_dev_t *netdev, int needed_size, int *offset);
    int           (*packet_send)(net_dev_t *netdev, net_packet_t *p, ip4_addr_t ip_dst, int type);

    net_dev_t     *next;
};

// Ethernet devices should implement an eth_dev_t, a simple super
// class of net_dev_t.
typedef struct
{
    net_dev_t    netdev;
    eth_addr_t   eth_addr;
} eth_dev_t;

struct  __attribute__ ((packed)) ethernet_header
{
    uint8_t   srchw[6];
    uint8_t   dsthw[6];
    uint8_t   type[2];

    uint8_t   payload[0];
};

struct __attribute__ ((packed)) ip_header
{
    uint8_t  ver_hdrlen;
    uint8_t  tos;
    uint8_t  length[2];
    uint8_t  id[2];
    uint8_t  frag_offset[2];
    uint8_t  ttl;
    uint8_t  protocol;
    uint8_t  checksum[2];
    uint8_t  src_ip[4];
    uint8_t  dst_ip[4];

    uint8_t  payload[0];
};

struct __attribute__ ((packed)) icmp_header
{
    uint8_t  type;
    uint8_t  code;
    uint8_t  checksum[2];
    uint8_t  data[];
};

struct __attribute__ ((packed)) arp_header
{
    uint8_t  hw_type[2];
    uint8_t  prot_type[2];
    uint8_t  hw_size;
    uint8_t  prot_size;
    uint8_t  op[2];
    uint8_t  sender_eth_addr[6];
    uint8_t  sender_ip_addr[4];
    uint8_t  target_eth_addr[6];
    uint8_t  target_ip_addr[4];
};

struct __attribute__ ((packed)) udp_header
{
    uint8_t  src_port[2];
    uint8_t  dst_port[2];
    uint8_t  length[2];
    uint8_t  checksum[2];

    uint8_t  payload[0];
};

struct __attribute__ ((packed)) tcp_header
{
    uint8_t  src_port[2];
    uint8_t  dst_port[2];
    uint8_t  seq[4];
    uint8_t  ack[4];
    uint8_t  hdr_len;
    uint8_t  flags;
    uint8_t  win_size[2];
    uint8_t  checksum[2];
    uint8_t  urgent[2];

    uint8_t  payload[0];
};

typedef struct ip_alloc ip_alloc_t;
struct ip_alloc
{
    ip4_addr_t   dest_ip_addr; // where will this packet be transmitted to?
    ip_config_t *ipcfg;        // what interface will we send it over?

    net_packet_t *p;           // the packet, allocated by the interface
    struct ip_header *ip_hdr;
};

typedef struct udp_alloc udp_alloc_t;
struct udp_alloc
{
    ip_alloc_t ipalloc;
    struct udp_header *udp_hdr;
};

typedef struct icmp_alloc icmp_alloc_t;
struct icmp_alloc
{
    ip_alloc_t ipalloc;
    struct icmp_header *icmp_hdr;
};

typedef struct tcp_alloc tcp_alloc_t;
struct tcp_alloc
{
    ip_alloc_t ipalloc;
    struct tcp_header *tcp_hdr;
};


static inline uint16_t ntohs(uint16_t v)
{
    return ((v & 0x00ff) << 8) | ((v & 0xff00) >> 8);
}

#define htons ntohs

static inline uint32_t ntohl(uint32_t v)
{
    return ((v & 0x000000ff) << 24) | ((v & 0x0000ff00) << 8) |
      ((v & 0x00ff0000) >> 8) | ((v & 0xff000000) >> 24);
}

typedef struct ip_packet
{
    ip4_addr_t ip_addr;
    net_dev_t  *netdev;

} ip_packet;

#define htonl ntohl

uint16_t ip_checksum(void *p, uint32_t len, uint32_t acc);

void      net_packet_dump(net_packet_t *packet);

void      net_init();

void      net_ethernet_receive(net_packet_t *p, int offset);
void      net_ip_receive(net_packet_t *p, int offset);
void      net_icmp_receive(net_packet_t *p, struct ip_header *ip_hdr, int offset);
void      net_udp_receive(net_packet_t *p, struct ip_header *ip_hdr, int offset);

/** Query the ARP database; if the eth address is unknown, a query is
 * initiated and -1 is returned. If the address is known, eth_address
 * is filled in and 0 is returned. **/
int net_arp_query(eth_dev_t *eth, uint32_t query_ip_addr, eth_addr_t *eth_addr);

/** Insert data into the ARP database. **/
int       net_arp_put(eth_dev_t *ethdev, uint32_t ip_addr, eth_addr_t eth_addr);

net_dev_t *net_route_get_device(ip4_addr_t ip_addr);

/** Allocate a packet large enough for an IP payload of needed_size
 * (IP header size will be added). The allocated packet, and the
 * offset at which the data must be written, are returned. Returns 0
 * on success. **/
int ip_packet_alloc(ip_alloc_t *ipalloc, ip4_addr_t dest_addr, int *offset, int needed_size);

int ip_header_fill(ip_alloc_t *ipalloc, int size, int protocol);

int ip_packet_send(ip_alloc_t *ipalloc);

/////////////////////////////////////////////////////////////////
// Network byte order encode/decode functions
void net_encode_ip_addr(uint8_t *p, uint32_t ip_addr);
ip4_addr_t net_decode_ip_addr(uint8_t *p);
void net_encode_eth_addr(uint8_t *p, eth_addr_t eth_addr);
void net_decode_eth_addr(uint8_t *p, eth_addr_t *_addr);
void net_encode_u8(uint8_t *p, uint8_t v);
uint8_t net_decode_u8(uint8_t *p);
void net_encode_u16(uint8_t *p, uint16_t v);
uint16_t net_decode_u16(uint8_t *p);
#define net_encode_u32 net_encode_ip_addr
#define net_decode_u32 net_decode_ip_addr
void net_encode_u64(uint8_t *p, uint64_t v);
uint64_t net_decode_u64(uint8_t *p);


void net_arp_receive(net_packet_t *p, int offset);


#define NET_PACKET_IP   0x8000
#define NET_PACKET_ARP  0x8006


#define IP_HEADER_SIZE 20
#define UDP_HEADER_SIZE 8
#define ICMP_HEADER_SIZE   4
#define TCP_HEADER_SIZE 20

#define IP_PROTOCOL_ICMP 0x01
#define IP_PROTOCOL_UDP 0x11
#define IP_PROTOCOL_TCP 0x06

// each listener gets this many rxdata buffers. (not *packets*, but
// pointers to packets. It makes no sense for this to be larger than
// the number of packets.)
#define UDP_RXDATA_RECORDS 4

typedef struct udp_rxdata udp_rxdata_t;
struct udp_rxdata
{
    net_packet_t *p;
    struct ip_header *ip_hdr;
    struct udp_header *udp_hdr;
    uint8_t state; // 0 = empty. 1 = received and waiting. 2 = received and returned to user.
};

typedef struct udp_listener udp_listener_t;
struct udp_listener
{
    uint32_t port;
    nkern_semaphore_t   rx_sem;

    udp_rxdata_t rxdata[UDP_RXDATA_RECORDS];

    udp_listener_t *next;

    int rx_packet_count; // for statistics only
};

typedef struct tcp_connection tcp_connection_t;
typedef struct tcp_listener tcp_listener_t;

// the TCP functions are works in progress and are not implemented.
void tcp_init();

/** Create a new listener. **/
tcp_listener_t *tcp_listen(int port);

/** Wait for a new connection for a previously-created listener. **/
tcp_connection_t *tcp_accept(tcp_listener_t *tcplistener);

/** Initiate a new out-going connection. **/
tcp_connection_t *tcp_connect(ip4_addr_t host, int port);

int tcp_putc(tcp_connection_t *tcp, char c);
int tcp_getc(tcp_connection_t *tcp);

/** Retrieve the iop_t structure that is part of this connection. **/
iop_t *tcp_get_iop(tcp_connection_t *con);

/** Flush the transmit buffer. Characters are not guaranteed to be
    transmitted at all until flush() is called. (This is different
    than POSIX). Returns -1 if the connection has failed. **/
int tcp_flush(tcp_connection_t *tcp);

/** Close the tcp connection for future writing (performing a
 * half-close). Future reads are still allowed, and the resources for
 * the connection are NOT freed.
 **/
void tcp_close(tcp_connection_t *tcp);
void tcp_print_stats(iop_t *iop);

/** Free resources associated with connection. **/
void tcp_free(tcp_connection_t *tcp);

int udp_listen(udp_listener_t *listener, uint16_t port);
int udp_read(udp_listener_t *listener, udp_rxdata_t **rxdata);
int udp_read_release(struct udp_listener *listener, udp_rxdata_t *rxdata);
void udp_print_stats(iop_t *iop);

/** Sending UDP packets is a three step process.

    1. You must allocate the packet first, providing the destination
    and size. (This is so that a DMA-able memory buffer can be
    returned to you for the correct network device.

    2. Get the packet pointer with udp_packet_get_buffer, and write the data.

    3. Send the packet with udp_packet_send. You may *reduce* the size
    of the packet.
**/
int udp_packet_alloc(udp_alloc_t *udpalloc, ip4_addr_t dest_addr, int *offset, int needed_size);
uint8_t *udp_packet_get_buffer(udp_alloc_t *udpalloc);
int udp_packet_send(udp_alloc_t *udpalloc, int size, int src_port, int dst_port);


void net_tcp_receive(net_packet_t *p, struct ip_header *ip_hdr, int offset);

int ip_config_add(net_dev_t *netdev, ip4_addr_t ip_addr, ip4_addr_t ip_mask, ip4_addr_t gw_ip_addr);

ip_config_t *ip_route(ip4_addr_t addr);
int ip_is_local_address(ip4_addr_t addr);
ip4_addr_t ip_primary_address();

int icmp_packet_alloc(icmp_alloc_t *icmpalloc, ip4_addr_t dest_addr, int *offset, int needed_size);
int icmp_packet_send(icmp_alloc_t *icmpalloc, int size);

ip4_addr_t inet_addr(const char *s);
void inet_addr_to_dotted(ip4_addr_t addr, char *s); // s should have at least 16 characters space

//void memcpy_chunk(net_chunk_t *chunk, int offset, void *_src, int size);

#endif
