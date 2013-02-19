#include <nkern.h>

#include <lpc23xx.h>
#include <platform.h>

#define LCD_E     0x80000000            /* Enable control pin                */
#define LCD_RW    0x20000000            /* Read/Write control pin            */
#define LCD_RS    0x10000000            /* Data/Instruction control          */
#define LCD_CTRL  0xB0000000            /* Control lines mask                */
#define LCD_DATA  0x0F000000            /* Data lines mask                   */

static int x_pos, y_pos;
static int height = 2, width = 16;
static int scroll_pending = 0;

static nkern_mutex_t iomutex;

// original divisor: 37
#define nsleep(x) { asm volatile ("            mov r0, #" #x "/ 37 + 1      \r\n" \
                                  "_nsleep_%=: subs r0, r0, #1      \r\n" \
                                  "            bmi  _nsleep_%=                     \r\n" \
                                  : : : "r0"); }

static void charlcd_write_4bit(int c)
{
    /* Write a 4-bit command to LCD controller. */
    FIO1DIR |= LCD_DATA | LCD_CTRL;
    FIO1CLR  = LCD_RW   | LCD_DATA;
    FIO1SET  = (c & 0xF) << 24;
    FIO1SET  = LCD_E;
    // minimum E pulse width (Tpw) = 150ns
    nsleep(150);
    FIO1CLR  = LCD_E;

    // hold time
    nsleep(10); 
    return;
}

static void charlcd_write( int c ) 
{
    /* Write data/command to LCD controller. */
    charlcd_write_4bit (c >> 4);
    charlcd_write_4bit (c);
    return;
}

static int charlcd_read_stat( void ) 
{
    /* Read status of LCD controller (ST7066) */
    int stat;

    FIO1DIR &= ~LCD_DATA;
    FIO1CLR  = LCD_RS;
    FIO1SET  = LCD_RW;
    nsleep(30); // Tas
    FIO1SET  = LCD_E;
    nsleep(100); // Tddr
    stat    = (FIO1PIN >> 20) & 0xF0;
    FIO1CLR  = LCD_E;
    nsleep(10); // hold time

    // I thought maybe we could skip reading the low 4 bits, since 
    // we're usually only interested in the busy flag, but this
    // doesn't work.
    nsleep(260); // cycle time
    FIO1SET  = LCD_E;
    nsleep(100); // Tddr
    stat   |= (FIO1PIN >> 24) & 0xF;
    FIO1CLR  = LCD_E;

    return stat;
}

static void charlcd_wait_busy( void ) 
{
    /* Wait until LCD controller (ST7066) is busy. */
    int timeout = 0;
    while (1)
    {
        int stat = charlcd_read_stat();
        if (!(stat & 0x80))
            break;

        nkern_yield();

        if (timeout++ > 10)
            return;
    }
}

static void charlcd_write_cmd(int c)
{
    charlcd_wait_busy();
    FIO1CLR = LCD_RS;
    charlcd_write(c);
}

static void charlcd_write_data(int d) 
{
    /* Write data to LCD controller. */
    charlcd_wait_busy();
    FIO1SET = LCD_RS;
    charlcd_write(d);
}

// zero-indexed.
void charlcd_goto(int x, int y)
{
    nkern_mutex_lock(&iomutex);

    int v = y ? 0x40 : 0;
    v+=x;

    charlcd_write_cmd(v | 0x80);

    x_pos = x;
    y_pos = y;
    scroll_pending = 0;

    nkern_mutex_unlock(&iomutex);
}

void charlcd_clear()
{  
    charlcd_write_cmd (0x01);
    charlcd_goto (0, 0);
    scroll_pending = 0;

    nkern_usleep(1600); // this is a very slow operation.
}

void charlcd_on()
{
    charlcd_write_cmd(0x0e);
}

void charlcd_cursor_off()
{
    charlcd_write_cmd(0x0c);
}

static void charlcd_putc_raw(char c)
{
    charlcd_write_data(c);
}

static void clear_line()
{
    // clear the rest of this line
    for (int i = x_pos; i < width; i++)
        charlcd_putc_raw(' ');

    charlcd_goto(x_pos, y_pos);
}

static void scroll()
{
    // clear the rest of this line
    for (int i = x_pos; i < width; i++)
        charlcd_putc_raw(' ');

    // move to next line
    x_pos = 0;
    y_pos++;
    if (y_pos == height)
        y_pos = 0;

    charlcd_goto(0, y_pos);
}

void charlcd_putc( char c ) 
{ 
    nkern_mutex_lock(&iomutex);

    if (c=='\n' || c=='\r') {
        clear_line();
        scroll_pending = 1;

        goto done;
    }

    if (scroll_pending || x_pos == width) {
        scroll();
        scroll_pending = 0;
    }
    
    charlcd_putc_raw(c);
    x_pos ++;

done:
    nkern_mutex_unlock(&iomutex);
    return;
}

void charlcd_puts(const char *s)
{
    while (*s) {
        charlcd_putc(*s);
        s++;
    }
}

long charlcd_write_fd(void *user, const void *data, int len)
{
    (void) user;

    for (int i = 0; i < len; i++)
        charlcd_putc(((char*)data) [i]);
    return len;
}

void charlcd_init()
{
    nkern_mutex_init(&iomutex, "clcd");

    /* Initialize the ST7066 LCD controller to 4-bit mode. */
    uint32_t mask =  (3<<16 | 3<<18 | 3<<20 | 3<<22 | 3<<24 | 3<<26 | 3<<30);
    uint32_t vals =  (0<<16 | 0<<18 | 0<<20 | 0<<22 | 0<<24 | 0<<26 | 0<<30);
    PINSEL3 = (PINSEL3 & (~mask)) | vals;
        
    FIO1DIR |= LCD_CTRL | LCD_DATA;
    FIO1CLR  = LCD_RW   | LCD_RS   | LCD_DATA;

    charlcd_write_4bit(0x3);                /* Select 4-bit interface            */
    nkern_usleep(5000);
    charlcd_write_4bit(0x3);
    nkern_usleep(5000);
    charlcd_write_4bit(0x3);
    charlcd_write_4bit(0x2);

    charlcd_write_cmd(0x28);                /* 2 lines, 5x8 character matrix     */
    charlcd_write_cmd(0x0e);                /* Display ctrl:Disp/Curs/Blnk=ON    */
    charlcd_write_cmd(0x06);                /* Entry mode: Move right, no shift  */

    charlcd_clear();
    charlcd_goto(0,0);
    charlcd_cursor_off();

    return;

}

