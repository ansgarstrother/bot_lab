#include <string.h>
#include <stdlib.h>

#include <nkern.h>
#include <net.h>

struct dhcp_message
{
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint8_t xid[4];
    uint8_t secs[2];
    uint8_t flags[2];
    
    uint8_t ciaddr[4];
    uint8_t yiaddr[4];
    uint8_t siaddr[4];
    uint8_t giaddr[4];
    
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];

    uint8_t cookie[4];
    uint8_t options[312];
};

#define DHCP_BOOT_REQUEST 1
#define DHCP_HWTYPE_ETHERNET 1
#define DHCP_HWLEN_ETHERNET 6

#define DHCP_FLAG_BROADCAST 0x0001

#define DHCP_MAGIC_COOKIE 0x63825363

void dhcp(eth_dev_t *eth)
{
    struct dhcp_message req;

    bzero(&req, sizeof(struct dhcp_message));

    req.op = DHCP_BOOT_REQUEST;
    req.htype = DHCP_HWTYPE_ETHERNET;
    req.hlen  = DHCP_HWLEN_ETHERNET;
    req.hops = 0;
    req.xid[0] = rand();  req.xid[1] = rand();  req.xid[2] = rand();  req.xid[3] = rand();
    req.secs[0] = 0; req.secs[1] = 0;
    net_encode_u16(req.flags, DHCP_FLAG_BROADCAST);
    net_encode_ip_addr(req.ciaddr, 0);
    net_encode_ip_addr(req.yiaddr, 0);
    net_encode_ip_addr(req.siaddr, 0);
    net_encode_ip_addr(req.giaddr, 0);
    net_encode_eth_addr(req.chaddr, eth->eth_addr);

    net_encode_u32(req.cookie, DHCP_MAGIC_COOKIE);

    int opt_pos = 0;
    req.options[opt_pos++] = 0x35; // DHCP Message type
    req.options[opt_pos++] = 0x01; // len
    req.options[opt_pos++] = 0x03; // DHCP request

    req.options[opt_pos++] = 0xff; // end of options
}
