#ifndef _USB_H
#define _USB_H

#include <nkern.h>

// OUT = host to device
// IN = device to host
#define CONTROL_OUT_EP  0
#define CONTROL_IN_EP   1

struct epstate
{
    volatile void              *p;    // all of our transfers are words; this buffer must be word aligned
    volatile int                len;
    volatile int                pos;
    volatile int                maxlen;
    nkern_task_list_t           waitlist;
    volatile uint8_t            hasdata;      
};

extern struct epstate epstates[32];

typedef void (*usb_control_out_hook_t)(uint16_t wRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength);

void usb_init(const char *devDescriptor, 
              const char *cfgDescriptor, 
              const char * const *strDescriptor, 
              usb_control_out_hook_t hook);
void usb_reset();

// initialize the usb stack for traffic on a given endpoint
void usb_endpoint_init(int physical_ep, int maxlen, const char *name);

// ask the hardware to make a previously initialized ep functional.
void usb_endpoint_realize(int physical_ep);

void usb_set_address(int address);

void usb_write_begin(int physical_ep, const void *p, int len);
void usb_write(int physical_ep, const void *p, int len);

// there's no len field; you must pass a buffer big enough for maxlen data
// where maxlen was specified by usb_create_endpoint
//void usb_read_begin(int physical_ep, void *p);

int usb_read(int physical_ep, void *p);

//#define USB_DBG(...) kprintf(__VA_ARGS__);
#define USB_DBG(...) 

#endif
