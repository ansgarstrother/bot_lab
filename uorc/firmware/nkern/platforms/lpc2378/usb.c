#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <lpc23xx.h>
#include <nkern.h>

#include "usb.h"

#define PCUSB (1<<31)
#define EP_RLZED (1<<8)

#define USB_DEV_INT_FRAME    (1<<0)
#define USB_DEV_INT_EP_FAST  (1<<1)
#define USB_DEV_INT_EP_SLOW  (1<<2)
#define USB_DEV_INT_DEV_STAT (1<<3)
#define USB_DEV_INT_CCEMPTY  (1<<4)
#define USB_DEV_INT_CCFULL   (1<<5)
#define USB_DEV_INT_RXENDPKT (1<<6)
#define USB_DEV_INT_TXENDPKT (1<<7)
#define USB_DEV_INT_EP_RLZED (1<<8)
#define USB_DEV_INT_ERR_INT  (1<<9)

#define CCEMPTY 0x10
#define CCFULL  0x20

#define DEVSTAT_CON (1<<0)
#define DEVSTAT_CON_CHANGED (1<<1)
#define DEVSTAT_SUS (1<<2)
#define DEVSTAT_SUS_CHANGED (1<<3)
#define DEVSTAT_RESET (1<<4)

#define RD_EN 1
#define WR_EN 2

#define STD_GET_STATUS_ZERO           0x0080
#define STD_GET_STATUS_INTERFACE      0x0081
#define STD_GET_STATUS_ENDPOINT       0x0082

#define STD_CLEAR_FEATURE_ZERO        0x0100
#define STD_CLEAR_FEATURE_INTERFACE   0x0101
#define STD_CLEAR_FEATURE_ENDPOINT    0x0102

#define STD_SET_FEATURE_ZERO          0x0300
#define STD_SET_FEATURE_INTERFACE     0x0301
#define STD_SET_FEATURE_ENDPOINT      0x0302

#define STD_SET_ADDRESS               0x0500
#define STD_GET_DESCRIPTOR            0x0680
#define STD_SET_DESCRIPTOR            0x0700
#define STD_GET_CONFIGURATION         0x0880
#define STD_SET_CONFIGURATION         0x0900
#define STD_GET_INTERFACE             0x0A81
#define STD_SET_INTERFACE             0x0B01
#define STD_SYNCH_FRAME               0x0C82

struct epstate epstates[32];

void usb_control_init(); // called to initialize EP0 and EP1
void usb_control_out();  // called upon receipt of data from the host
void usb_control_ack();  // called when we've acked a message from the host

static const char *devDescriptor, *cfgDescriptor;
static const char * const * strDescriptor;
static usb_control_out_hook_t control_out_hook;

static int set_usb_address;
int usb_address;
int usb_configuration;

static char __attribute__ ((aligned (4))) control_readbuf[64];

static inline int min(int a, int b)
{
    return a < b ? a : b;
}

void usb_endpoint_init(int physical_ep, int maxlen, const char *name)
{
    epstates[physical_ep].maxlen = maxlen;
    epstates[physical_ep].len = 0;
    epstates[physical_ep].pos = 0;
    epstates[physical_ep].hasdata = 0;
    nkern_task_list_init(&epstates[physical_ep].waitlist, name);
}

void usb_endpoint_realize(int physical_ep)
{
    DEV_INT_CLR = EP_RLZED;

    REALIZE_EP |= (1 << physical_ep);
    EP_INDEX = physical_ep;
    MAXPACKET_SIZE = epstates[physical_ep].maxlen;

    while (!(DEV_INT_STAT & EP_RLZED));

    DEV_INT_CLR = EP_RLZED;

    EP_INT_EN |= (1<<physical_ep);
}

void usb_cmd_read(uint8_t _cmd_code, uint8_t *data, int datalen)
{
    uint32_t cmd_code = _cmd_code << 16;

    DEV_INT_CLR = CCEMPTY;             // clear CCEMPTY && CDFULL
    CMD_CODE = 0x0500 | cmd_code;      // cmd phase = command
    while (!(DEV_INT_STAT & CCEMPTY)); // wait for CCEMPTY

    for (int i = 0; i < datalen; i++)
    {
        int32_t t;
        DEV_INT_CLR = CCFULL;
        CMD_CODE = 0x200 | cmd_code;      // cmd phase = read
        while (!(DEV_INT_STAT & CCFULL)); // wait for CCFULL
        t = CMD_DATA;
        data[i] = t;
    }
}

uint8_t usb_cmd_read_8(uint8_t cmd_code)
{
    uint8_t v;
    usb_cmd_read(cmd_code, &v, 1);
    return v;
}

void usb_cmd_write_nodata(uint8_t _cmd_code)
{
    uint32_t cmd_code = _cmd_code << 16;

    DEV_INT_CLR = CCEMPTY;    // clear CCEMPTY && CDFULL
    CMD_CODE = 0x0500 | cmd_code;      // cmd phase = command
    while (!(DEV_INT_STAT & CCEMPTY)); // wait for CCEMPTY
}

void usb_cmd_write(uint8_t _cmd_code, uint8_t _value)
{
    uint32_t cmd_code = _cmd_code << 16;
    uint32_t value = _value << 16;

    DEV_INT_CLR = CCEMPTY;    // clear CCEMPTY && CDFULL
    CMD_CODE = 0x0500 | cmd_code;      // cmd phase = command
    while (!(DEV_INT_STAT & CCEMPTY)); // wait for CCEMPTY

    DEV_INT_CLR = CCEMPTY;             // clear CCEMPTY
    CMD_CODE = 0x0100 | value;
    while (!(DEV_INT_STAT & CCEMPTY)); // wait for CCEMPTY
}

void usb_set_address(int addr)
{
    // We set this twice to make sure that
    // the change is immediate.
    usb_cmd_write(0xd0, 0x80 | addr);
    usb_cmd_write(0xd0, 0x80 | addr); 
}

void usb_reset()
{
    EP_INDEX = 0;
    MAXPACKET_SIZE = 64;
    EP_INDEX = 1;
    MAXPACKET_SIZE = 64;
    while (!(DEV_INT_STAT & USB_DEV_INT_EP_RLZED));

    //8. Enable EP interrupts
    EP_INT_CLR = 0xffffffff;  // clear all pending EP interrupts
    while (!(DEV_INT_STAT & CCFULL));

    EP_INT_EN  = 0xffffffff;
    DEV_INT_CLR = 0xffffffff; // clear all pending DEV interrupts
    DEV_INT_EN = USB_DEV_INT_DEV_STAT | USB_DEV_INT_EP_SLOW | USB_DEV_INT_EP_FAST;
}

static const char log_table[] = 
{
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

static inline int int_log_2(uint32_t v)
{
    uint32_t r;     // r will be lg(v)
    register uint32_t t, tt; // temporaries
    
    if ((tt = v >> 16))
    {
        r = (t = tt >> 8) ? 24 + log_table[t] : 16 + log_table[tt];
    }
    else 
    {
        r = (t = v >> 8) ? 8 + log_table[t] : log_table[v];
    }    
    
    return r;
}

/** We use interrupts to indicate that we should check to see if
    there's something to do. Standard hardware interrupts, of course,
    indicate that there might be work to do. However, if a user calls
    usb_read() or usb_write(), that creates a change too because a new
    buffer is available. We trigger an interrupt manually so that we
    can process the request.
**/
static void usb_irq_real(void) __attribute__ ((noinline));
static void usb_irq_real()
{
    int v = DEV_INT_STAT;
    int reschedule = 0;

    DEV_INT_CLR = v;

    USB_DBG("EP_STAT: %08x   ", EP_INT_STAT);

    if (v & USB_DEV_INT_DEV_STAT)
    {
        uint8_t devstat = usb_cmd_read_8(0xfe);
        DEV_INT_CLR = USB_DEV_INT_DEV_STAT;

        if (devstat & DEVSTAT_RESET) {
            USB_DBG("RESET");
            usb_reset();
        }
    }

    // Handle endpoint interrupts
    if (v & USB_DEV_INT_EP_SLOW)
    {
        uint32_t epstat = EP_INT_STAT;

        // loop over each bit that is set in epstat
        for ( ; epstat ; epstat &= (epstat-1) )
        {
            int32_t mask = epstat^(epstat&(epstat-1)); // XXX can be simplified, probably.
            int     physical_ep = int_log_2(mask);
            int     logical_ep  = physical_ep / 2;

            EP_INT_CLR = mask;                // clear this interrupt
            while (!(DEV_INT_STAT & CCFULL));

            if (physical_ep & 1)              // IN endpoint (to host)
            {
                // make sure there's data to transmit
                if (!epstates[physical_ep].hasdata)
                {
                    // there's no data, but maybe it's time to set the new usb address
                    if (physical_ep == 1 && set_usb_address)
                    {
                        USB_DBG("SET ADDR %d", usb_address);
                        usb_set_address(usb_address);
                        set_usb_address = 0;
                    }
                }
                else
                {
                    while (1)
                    {
                        // make sure there's room to transmit
                        int selep = usb_cmd_read_8(physical_ep);
                        if (selep & 1)
                            break;
                        
                        int thislen = min(epstates[physical_ep].maxlen, epstates[physical_ep].len - epstates[physical_ep].pos);
                        
                        USB_CTRL = WR_EN | (logical_ep << 2);
                        TX_PLENGTH = thislen;
                        
                        USB_DBG("WRITE %d: %d", physical_ep, thislen);
                        
                        if (thislen == 0)
                            TX_DATA = 0;
                        
                        uint32_t *p = (uint32_t*) epstates[physical_ep].p;
                        for (int i = 0; i < (thislen+3)/4; i++)
                            TX_DATA = p[i + epstates[physical_ep].pos/4];
                        
                        usb_cmd_write_nodata(0xfa);      // validate buffer
                        
                        epstates[physical_ep].pos += thislen;
                        if (epstates[physical_ep].len == epstates[physical_ep].pos) 
                        {
                            epstates[physical_ep].hasdata = 0;
                            
                            reschedule |= _nkern_wake_all(&epstates[physical_ep].waitlist);
                            
                            break;
                        }
                    }
                }
            }
            else                              // OUT endpoint (from host)
            {
                // get ready to read. (always read the whole
                // packet. it is an error for the device to advertise
                // a maximum packet length greater than the device's
                // buffer.)

                do {
                    // user hasn't read out previous data!
                    if (epstates[physical_ep].hasdata) {
                        USB_DBG("HAS DATA ");
                        break;
                    }
                    
                    // any data in the buffer?
                    int selep = usb_cmd_read_8(physical_ep);
                    if (!(selep & 1)) {
                        USB_DBG("NO DATA ");
                        break;
                    }
                    
                    USB_CTRL = RD_EN | (logical_ep << 2);
                    while (!(RX_PLENGTH & (1<<11)));
                    
                    // read the data
                    int len = RX_PLENGTH & 0x3ff;
                    
                    if (len == 0)
                    {
                        (void) RX_DATA;
                    }

                    USB_DBG("READ  %d: %d", physical_ep, len);

                    uint32_t *p = (uint32_t*) epstates[physical_ep].p;
                    for (int i = 0; i < (len+3) / 4; i++)
                        p[i] = RX_DATA;
                    
                    usb_cmd_read_8(0xf2);             // clear buffer
                    
                    epstates[physical_ep].pos = 0;
                    epstates[physical_ep].len = len;
                    epstates[physical_ep].hasdata = 1;
                    
                    reschedule |= _nkern_wake_all(&epstates[physical_ep].waitlist);
                    
                    if (physical_ep == 0)
                        usb_control_out();

                } while (0);
            }
        }
    }

    USB_DBG("\n");

    VICVectAddr = 0;
    
    if (reschedule)
      _nkern_schedule();
}

void usb_irq(void) __attribute__ ((naked));
void usb_irq(void)
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;
    usb_irq_real();
    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}

/** Begin a read operation, without waiting for the response. This
    should only be used by the control endpoint. **/
static void usb_read_begin(int physical_ep, void *p)
{
    epstates[physical_ep].p = p;
    epstates[physical_ep].len = 0;
    epstates[physical_ep].pos = 0;
    epstates[physical_ep].hasdata = 0;

    EP_INT_SET |= (1 << physical_ep);
}

/** Wait for new data to arrive on the end point. The buffer p must be
 * sufficiently large. The calling thread will be put to sleep until
 * data is available; do not call from an interrupt context. 
 **/
int usb_read(int physical_ep, void *p)
{
    irqstate_t state;

    disable_interrupts(&state);
    usb_read_begin(physical_ep, p);
    nkern_wait(&epstates[physical_ep].waitlist);
    enable_interrupts(&state);

    return epstates[physical_ep].len;
}

/** Begin a write operation, without waiting for completion. This
 * should only be used in responses to SETUP packets on EP 0. 
 **/
void usb_write_begin(int physical_ep, const void *p, int len)
{
    epstates[physical_ep].p = (void*) p;
    epstates[physical_ep].len = len;
    epstates[physical_ep].pos = 0;
    epstates[physical_ep].hasdata = 1;

    EP_INT_SET |= (1 << physical_ep);
}

/** Begin a write operation, waiting until the data has been
 * successfully transferred to the USB controller's buffer. The
 * current thread is put to sleep; do not call this from an interrupt
 * context. 
 **/
void usb_write(int physical_ep, const void *p, int len)
{
    irqstate_t state;

    disable_interrupts(&state);
    usb_write_begin(physical_ep, p, len);
    nkern_wait(&epstates[physical_ep].waitlist);
    enable_interrupts(&state);

    return;
}

void usb_control_out()
{
    uint16_t *p = (uint16_t*) epstates[0].p;

    uint16_t wRequest = p[0];
    uint16_t wValue   = p[1];
    uint16_t wIndex   = p[2];
    uint16_t wLength  = p[3];

    USB_DBG("      CONTROL: %d %04x %04x", epstates[0].len, wRequest, wValue);

    if (epstates[CONTROL_OUT_EP].len == 0)
      goto exit;

    control_out_hook(wRequest, wValue, wIndex, wLength);

    switch (wRequest)
    {
    case STD_GET_DESCRIPTOR:
        
        switch (wValue & 0xff00)
        {
        case 0x100: // device descriptor
            usb_write_begin(CONTROL_IN_EP, devDescriptor, min(wLength, devDescriptor[0]));
            break;

        case 0x200: // configuration
            usb_write_begin(CONTROL_IN_EP, cfgDescriptor, wLength);
            break;

        case 0x300: // string
            usb_write_begin(CONTROL_IN_EP, strDescriptor[wValue & 0x00ff], min(wLength, strDescriptor[wValue & 0x00ff][0]));
            break;
        }
        break;
        
    case STD_SET_ADDRESS:
        /* Some annoying subtlety: we cannot set the USB address here;
           we have to write a zero-length packet pack and accept the
           ACK from the host. If we change our address now, we won't
           recognize the ACK and enumeration will fail. Thus, we set a
           flag to remind us to change the address when we get the ack.
        */
        usb_address = wValue;
        set_usb_address = 1;

        usb_write_begin(CONTROL_IN_EP, NULL, 0);
        break;

    case STD_SET_CONFIGURATION:
        usb_cmd_write(0xd8, 1);
        usb_write_begin(CONTROL_IN_EP, NULL, 0);
        break;
    }

 exit:
    // we're ready for more data.
    usb_read_begin(CONTROL_OUT_EP, control_readbuf);
}

/** Initialize the USB subsystem; call exactly once. **/
void usb_init(const char *_devDescriptor, 
              const char *_cfgDescriptor, 
              const char * const *_strDescriptor, 
              usb_control_out_hook_t hook)
{
    control_out_hook = hook;
    devDescriptor = _devDescriptor;
    cfgDescriptor = _cfgDescriptor;
    strDescriptor = _strDescriptor;

    PCONP |= PCUSB; 

    OTG_CLK_CTRL = 0x13;
    while ((OTG_CLK_STAT & 0x13) != 0x13);

    //5. enable USB pins
    PINSEL1 &= ~((3<<26) | (3<<28)); // U1D+ U1D-
    PINSEL1 |= (1<<26) | (1<<28); 

    PINSEL3 &= ~(3<<4); // U1UP_LED
    PINSEL3 |= (1<<4);  

    PINMODE3 &= ~(3 << 28); // VBUS is port 1.30, disable pullup/pulldown
    PINMODE3 |= (2 << 28); 

    PINSEL4 &= ~(3 << 18); // U1CONNECT on P2.9
    FIO2DIR |= 1 << 9;
    FIO2CLR = 1 << 9 ;

    // surprise! LPC2378 has broken implementation of U1CONNECT/VBUS; we must
    // work around by using these pins as GPIO
//    PINSEL4 |= (1<<18); // U1CONNECT
//    PINSEL3 |= (1<<28); // VBUS
    
    //6. Disable VBUS pullup

    ////////// initialize control end point
    usb_endpoint_init(CONTROL_IN_EP,  8, "usb-ctl-in");
    usb_endpoint_init(CONTROL_OUT_EP, 8, "usb-ctl-out");

    usb_endpoint_realize(CONTROL_IN_EP);
    usb_endpoint_realize(CONTROL_OUT_EP);

    usb_read_begin(CONTROL_OUT_EP, control_readbuf);

    set_usb_address = 0;
    usb_address = 0;
    usb_set_address(0);

    ///////////
    usb_reset();

    usb_cmd_write(0xfe, 0x01);

    //10. Install vector
    VICVectAddr22 = (uint32_t) usb_irq;
    VICIntEnable |= (1<<22);
    VICVectCntl22 = 5;

    return;
}

