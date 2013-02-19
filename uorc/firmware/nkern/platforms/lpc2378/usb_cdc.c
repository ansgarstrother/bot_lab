#include <string.h>
#include <usb.h>
#include <usb_cdc.h>

#define CDC_OUT_EP 4
#define CDC_IN_EP  5

/* CDC Class Specific Request Code */
#define GET_LINE_CODING               0x21A1
#define SET_LINE_CODING               0x2021
#define SET_CONTROL_LINE_STATE        0x2221

#define STD_SET_CONFIGURATION         0x0900

struct linecoding
{
    uint32_t baud;
    uint8_t stopbits;
    uint8_t parity;
    uint8_t databits;
} linecoding;

static const char __attribute__ ((aligned (4))) devDescriptor[] = {
        /* Device descriptor */
	18,   // bLength
        0x01,   // bDescriptorType
        0x10,   // bcdUSBL
        0x01,   //
        0x00,   // bDeviceClass:    CDC class code
        0x00,   // bDeviceSubclass: CDC class sub code
        0x00,   // bDeviceProtocol: CDC Device protocol
        0x08,   // bMaxPacketSize0 (was 8)
        0xed,   // idVendorL
        0x00,   //
        0x01,   // idProductL
        0x00,   //
        0x10,   // bcdDeviceL (vendor version)
        0x01,   //
        1,      // iManufacturer    // 0x01
        2,      // iProduct
        3,      // SerialNumber
        0x01,   // bNumConfigs
};

static const char __attribute__ ((aligned (4))) languages[] = {
	0x04, // bLength
	0x03, // bDescriptorType
	0x09, 0x04 // English
};

static const char __attribute__ ((aligned (4))) manufacturerStr[] = {
	14,     // bLength
	0x03, // bDescriptorType
	'E',0,'d',0,'C',0,'o',0,'r',0,'p',0,
};

static const char __attribute__ ((aligned (4))) productStr[] = {
	14,     // bLength
	0x03, // bDescriptorType
	'W',0,'i',0,'d',0,'g',0,'e',0,'t',0,
};

static const char __attribute__ ((aligned (4))) serialStr[] = {
	14,     // bLength
	0x03, // bDescriptorType
	'S',0,'0',0,'0',0,'0',0,'0',0,'1',0,
};

static const char * const strDescriptor[] = {
	languages, manufacturerStr, productStr, serialStr
};

static const char __attribute__ ((aligned (4))) cfgDescriptor[] = {
        /* ============== CONFIGURATION 1 =========== */
        /* Configuration 1 descriptor */
        0x09,   // CbLength
        0x02,   // CbDescriptorType
        9+9+7+7,   // CwTotalLength 2 EP + Control
        0x00,
        0x01,   // CbNumInterfaces
        0x01,   // CbConfigurationValue
        0x00,   // CiConfiguration
        0xC0,   // CbmAttributes 0xA0
        0x00,   // CMaxPower

       /* Data Class Interface Descriptor Requirement */
        0x09, // bLength
        0x04, // bDescriptorType
        0x00, // bInterfaceNumber
        0x00, // bAlternateSetting
        0x02, // bNumEndpoints
        0xff, // bInterfaceClass
        0xff, // bInterfaceSubclass
        0xff, // bInterfaceProtocol
        0x02, // iInterface

        /* Endpoint 1 descriptor */
        0x07,   // bLength
        0x05,   // bDescriptorType
        0x02,   // bEndpointAddress, Endpoint 02 - OUT (physical 4)
        0x02,   // bmAttributes      BULK
        0x40,   // wMaxPacketSize
        0x00,
        0x01,   // bInterval

        /* Endpoint 2 descriptor */
        0x07,   // bLength
        0x05,   // bDescriptorType
        0x82,   // bEndpointAddress, Endpoint 02 - IN (physical 5)
        0x02,   // bmAttributes      BULK
        0x40,   // wMaxPacketSize
        0x00,
        0x01    // bInterval
};

void usb_cdc_control_out(uint16_t wRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength)
{
    (void) wValue;
    (void) wIndex;
    (void) wLength;

    switch (wRequest)
    {
    case STD_SET_CONFIGURATION:
        usb_endpoint_realize(CDC_OUT_EP);
        usb_endpoint_realize(CDC_IN_EP);
        USB_DBG(" CDC REALIZED ");
        break;

    case GET_LINE_CODING:
        usb_write_begin(CONTROL_IN_EP, &linecoding, sizeof(struct linecoding));
        break;
        
    case SET_LINE_CODING:
        break;

    case SET_CONTROL_LINE_STATE:
        break;
    }
}

/** client must be able to accept a whole packet **/
int usb_cdc_read(void *p)
{
    return usb_read(CDC_OUT_EP, p);
}

char  __attribute__ ((aligned (4))) write_buffer[64];
int  write_buffer_length;

static inline int min(int a, int b)
{
    return a < b ? a : b;
}

long usb_cdc_write_fd(void *user, const void *data, int len)
{
    (void) user;
    usb_cdc_write(data, len);
    return len;
}

void usb_cdc_write(const void *_p, int len)
{
    irqstate_t state;
    int offset = 0;

    char *p = (char*) _p;

    // if (len==64) do a special case that doesn't require us to copy
    // the buffer??

    while (len)
    {
        disable_interrupts(&state);

        // did we already send to the USB layer a packet that has not
        // yet been handled?  if yes, then write_buffer_length will be
        // non-zero and we'll add to that packet.  otherwise, set
        // write_buffer_length to zero and start a new packet.
        if (!epstates[CDC_IN_EP].hasdata) 
            write_buffer_length = 0;

        // how many bytes to add to this packet?
        int thislen = min(64 - write_buffer_length, len);
        memcpy(&write_buffer[write_buffer_length], &p[offset], thislen);
        write_buffer_length += thislen;
        offset += thislen;
        len -= thislen;

        // if there are bytes yet to write, we must wait for this packet
        // to be sent before we can start the next packet. If all of the
        // bytes have been enqueued, we don't need to wait around: the bytes
        // have been copied and the usb layer will take care of them when
        // it has a chance.
        if (len)
            usb_write(CDC_IN_EP, write_buffer, write_buffer_length);
        else
            usb_write_begin(CDC_IN_EP, write_buffer, write_buffer_length);

        enable_interrupts(&state);
    }
}

void usb_cdc_init()
{
    usb_init(devDescriptor, cfgDescriptor, strDescriptor, usb_cdc_control_out);

    usb_endpoint_init(CDC_OUT_EP, 64, "cdc-out");
    usb_endpoint_init(CDC_IN_EP,  64, "cdc-in");
}

