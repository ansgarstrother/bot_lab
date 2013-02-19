#include <stdio.h>
#include <stdint.h>

#include <nkern.h>
#include <lpc23xx.h>
#include "serial.h"

#define NUM_PORTS 4

static nkern_wait_list_t rx_waitlist[NUM_PORTS];
static nkern_wait_list_t tx_waitlist[NUM_PORTS];

/**************************************************************************************************
cpuclk = 72mhz
  Peripheral clock : 18000000 Hz  

       300 : dll =  250   mulval = 0x1    divval = 0xE    ( actual baud =    300, error = 0.000 % )
      1200 : dll =  125   mulval = 0x2    divval = 0xD    ( actual baud =   1200, error = 0.000 % )
      2400 : dll =  125   mulval = 0x4    divval = 0xB    ( actual baud =   2400, error = 0.000 % )
      9600 : dll =   74   mulval = 0xC    divval = 0x7    ( actual baud =   9601, error = 0.013 % )
     19200 : dll =   37   mulval = 0xC    divval = 0x7    ( actual baud =  19203, error = 0.016 % )
     38400 : dll =   14   mulval = 0xB    divval = 0xC    ( actual baud =  38431, error = 0.082 % )
     57600 : dll =    8   mulval = 0x9    divval = 0xD    ( actual baud =  57528, error = 0.124 % )
    115200 : dll =    4   mulval = 0x9    divval = 0xD    ( actual baud = 115056, error = 0.124 % )
    230400 : dll =    2   mulval = 0x9    divval = 0xD    ( actual baud = 230113, error = 0.124 % )

cpuclk = 48mhz
Peripheral clock : 12000000 Hz

       300 : dll =  250   mulval = 0x1    divval = 0x9    ( actual baud =    300, error = 0.000 % )
      1200 : dll =  125   mulval = 0x1    divval = 0x4    ( actual baud =   1200, error = 0.000 % )
      2400 : dll =  125   mulval = 0x2    divval = 0x3    ( actual baud =   2400, error = 0.000 % )
      9600 : dll =   37   mulval = 0x9    divval = 0xA    ( actual baud =   9601, error = 0.016 % )
     19200 : dll =   17   mulval = 0xA    divval = 0xD    ( actual baud =  19181, error = 0.097 % )
     38400 : dll =    8   mulval = 0x9    divval = 0xD    ( actual baud =  38352, error = 0.124 % )
     57600 : dll =   11   mulval = 0xB    divval = 0x2    ( actual baud =  57691, error = 0.159 % )
    115200 : dll =    2   mulval = 0x4    divval = 0x9    ( actual baud = 115384, error = 0.160 % )
    230400 : dll =    2   mulval = 0x8    divval = 0x5    ( actual baud = 230769, error = 0.160 % )

***************************************************************************************************/

// return the base address for a given UART channel
#define UxBASE(port) ( port==0 ? UART0_BASE_ADDR : (port==1 ? UART1_BASE_ADDR : UART2_BASE_ADDR) )

// given the address of register in UART0's space, compute the corresponding address
#define UxPORT(port, offset) (*(volatile unsigned long *)( UxBASE(port) + offset))

void serial_putc_spin(int port, char c)
{
    while (!(UxPORT(port, UxLSR_OFFSET) & 0x20));

    UxPORT(port, UxTHR_OFFSET) = c;
}

int serial_getc_spin(int port)
{
    while (!(UxPORT(port, UxLSR_OFFSET) & 0x01));

    return UxPORT(port, UxRBR_OFFSET);
}

long serial_write(void *user, const void *data, int len)
{
    int port = (int) user;
    irqstate_t state;
    disable_interrupts(&state);

    for (int i = 0; i < len; i++)
    {
        // re-enable write interrupts
        UxPORT(port, UxIER_OFFSET) |= (1<<1);

        while (!(UxPORT(port, UxLSR_OFFSET) & 0x20)) {
            nkern_wait(&tx_waitlist[port]);
        }

        UxPORT(port, UxTHR_OFFSET) = ((char*) data)[i];
    }

    enable_interrupts(&state);

    return len;
}

long serial_read(void *user, void *data, int len)
{
    int port = (int) user;
    irqstate_t state;
    disable_interrupts(&state);
 
    (void) len;  // always read one character

    // re-enable read interrupts
    UxPORT(port, UxIER_OFFSET) |= (1<<0);

    while (!(UxPORT(port, UxLSR_OFFSET) & 0x01)) {
        nkern_wait(&rx_waitlist[port]);
    }

    ((char*) data)[0] = UxPORT(port, UxRBR_OFFSET);

    enable_interrupts(&state);

    return 1;
}

long serial_read_timeout(void *user, void *data, int len, uint64_t usecs)
{
    int port = (int) user;
    irqstate_t state;
    disable_interrupts(&state);
 
    (void) len;  // always read one character

    // re-enable read interrupts
    UxPORT(port, UxIER_OFFSET) |= (1<<0);

    if (!(UxPORT(port, UxLSR_OFFSET) & 0x01)) {
        if (nkern_wait_timeout(&rx_waitlist[port], usecs) < 0) {
            enable_interrupts(&state);
            return 0; // no data read, timeout.
        }
    }

    ((char*) data)[0] = UxPORT(port, UxRBR_OFFSET);
    
    enable_interrupts(&state);
    
    return 1;    
}

static void serial_irq_real(void) __attribute__ ((noinline));
static void serial_irq_real()
{
    uint32_t irq_flags = VICIRQStatus;
    int port;

    if (irq_flags & (1<<6))
        port = 0;
    else if (irq_flags & (1<<7))
        port = 1;
    else if (irq_flags & (1<<28))
        port = 2;
    else if (irq_flags & (1<<29))
        port = 3;
    else
        return;

    int event = UxPORT(port, UxIIR_OFFSET); // what irq occurred? (this clears the interrupt too.)
    int reschedule = 0;

    switch(event & 0x0f)
    {
    case 4:  // receive data available
    case 12: // character time-out indicator
        reschedule |= _nkern_wake_all(&rx_waitlist[port]);
        UxPORT(port, UxIER_OFFSET) &= ~(1<<0); // disable further read interrupts
        break;

    case 2:  // THRE
        reschedule |= _nkern_wake_all(&tx_waitlist[port]);
        UxPORT(port, UxIER_OFFSET) &= ~(1<<1); // disable further write interrupts
        break;

    case 6: // receive line status
        UxPORT(port, UxIER_OFFSET) &= ~(1<<2); // We don't do anything here
        break;

    case 0: // nothing
    default:
        break;
    }

    VICVectAddr = 0;

    if (reschedule)
      _nkern_schedule();

}

void serial_irq(void) __attribute__ ((naked));
void serial_irq(void)
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;
    serial_irq_real();
    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}

/** Convenience functions **/
void serial_putc(int port, char c)
{
    serial_write((void*) port, &c, 1);
}

int serial_getc(int port)
{
    char c;
    serial_read((void*) port, &c, 1);
    return c;
}

int serial_init(int port)
{
    nkern_wait_list_init(&rx_waitlist[port], "uart_rx");
    nkern_wait_list_init(&tx_waitlist[port], "uart_tx");

    switch (port) 
    {
    case 0:
        PINSEL0 &= ~( (3<<4) | (3<<6) );
        PINSEL0 |= (1<<4)  | (1<<6);
        break;
    case 1:
        PINSEL0 &= ~( (3<<30));
        PINSEL0 |= (1<<30);
        PINSEL1 &= ~( (3<<0));
        PINSEL1 |= (1<<0);
        break;
    case 2:
    case 3:
    default:
        return -1;
    }

    switch (port) 
    {
    case 0:
        VICVectAddr6 = (uint32_t) serial_irq;
        VICIntEnable |= (1<<6);
        VICVectCntl6 = 15; // very low priority
        break;
    case 1:
        VICVectAddr7 = (uint32_t) serial_irq;
        VICIntEnable |= (1<<7);
        VICVectCntl7 = 15; // very low priority
        break;
    case 2:
        VICVectAddr28 = (uint32_t) serial_irq;
        VICIntEnable |= (1<<28);
        VICVectCntl28 = 15; // very low priority
        break;

    case 3:
        VICVectAddr29 = (uint32_t) serial_irq;
        VICIntEnable |= (1<<29);
        VICVectCntl29 = 15; // very low priority
        break;
    }

    // enable read data avail and THRE isr
    UxPORT(port, UxIER_OFFSET) = (1<<0) | (1<<1);

    return 0;
}

// be smarter (and faster). Should produce exactly the same results as _brute_force.
// uses fixed-point arithmetic 
static void compute_baud(long pclk, int goalbaud, 
                         int *bestdll, int *bestmulval, int *bestdivval)
{
    int dll, mulval, divval;
    int besterr_256 = goalbaud*256;
    unsigned int goalbaud_256 = goalbaud*256;

    int max_dll = (pclk / goalbaud) / 16 + 1;
    int min_dll = (pclk / goalbaud) / 256 - 1;
    if (min_dll < 2)
        min_dll = 2;

    for (dll = min_dll; dll <= max_dll; dll++)
    {
        unsigned int ideal_frac_1024 = goalbaud * dll / (pclk / 1024 / 16);

        for (mulval = 0; mulval <= 15; mulval++) {

            unsigned int divval_1024 = (mulval*1024*1024 + ideal_frac_1024*1024/2) / (ideal_frac_1024) - mulval*1024;
            divval = divval_1024 / 1024;

            if (divval < 2 || divval > 15)
                continue;

            unsigned int baud_256 = pclk * 16 / dll;
            baud_256 = baud_256 * mulval / (mulval+divval);
            
            int err_256 = baud_256 - goalbaud_256;
            if (err_256 < 0)
                err_256 = -err_256;

            if (err_256 < besterr_256)
            {
                *bestdll = dll;
                *bestmulval = mulval;
                *bestdivval = divval;
                besterr_256 = err_256;
            }
        }
    }
}

int serial_configure(int port, int pclk, int baud)
{
    // 8N1, enable write to peripheral clock divider
    UxPORT(port, UxLCR_OFFSET) = 0x83; 

    /// put clock init here
    //    U0FDR = 0x9d; U0DLM = 0x00; U0DLL = 0x04;  // 72MHz CPU clk, 18MHz pclk
    int dll=0, mulval=0, divval=0;
    compute_baud(pclk, baud, &dll, &mulval, &divval);

    UxPORT(port, UxFDR_OFFSET) = (mulval<<4) | divval;
    UxPORT(port, UxDLM_OFFSET) = dll>>8;
    UxPORT(port, UxDLL_OFFSET) = dll&0xff;
    /// end clock init 

    // 8N1, disable write to peripheral clock divider (enable fifo reads)
    UxPORT(port, UxLCR_OFFSET) = 0x03;

    // enable FIFO, reset both RX and TX FIFOs, request
    // RX interrupt on a single character.
    UxPORT(port, UxFCR_OFFSET) = 0x07;

    return 0;
}


//////////////////////////////////////////////////////
// convenience functions
void serial_putc_0(char c)
{
    serial_putc(0, c);
}

void serial_putc_1(char c)
{
    serial_putc(1, c);
}

char serial_getc_0()
{
    return serial_getc(0);
}

char serial_getc_1()
{
    return serial_getc(1);
}
