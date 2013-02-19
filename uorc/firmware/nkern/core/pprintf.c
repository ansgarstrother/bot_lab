#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "nkern.h"

static inline void pprintf_double(iop_t *iop, int width, int leadingzero, int leftadjust, int precision, double v)
{
    char pprintf_tmp[64];

    int len = 0;
    int negative;

    if (v < 0) {
        negative = 1;
        v = -v;
    } else {
        negative = 0;
    }

    for (int i = 0; i < precision; i++)
        v *= 10;

    int iv = (int) v;

    do {
        int tv = iv%10;
        iv = iv / 10;
        pprintf_tmp[len++] = tv+'0';
        if (len == precision)
            pprintf_tmp[len++] = '.';
    } while (iv > 0 || (leadingzero && len < width - negative - 1)); // -1 for the '.'

    if (negative)
        pprintf_tmp[len++]='-';

    if (width > 0 && !leftadjust)
        for (int i = len ; i < width; i++)
            putc_iop(iop, ' ');

    for (int i = len-1; i >= 0; i--)
        putc_iop(iop, pprintf_tmp[i]);

    if (width > 0 && leftadjust)
        for (int i = len ; i < width; i++)
            putc_iop(iop, ' ');
}

static inline void pprintf_decimal(iop_t *iop, int width, int leadingzero, int leftadjust, int v)
{
    char pprintf_tmp[64];
    int len = 0;
    int negative;

    if (v < 0) {
        negative = 1;
        v = -v;
    } else {
        negative = 0;
    }

    do {
        int tv = v%10;
        v=v/10;
        pprintf_tmp[len++]=tv+'0';
    } while (v>0 || (leadingzero && len < width - negative));

    if (negative)
        pprintf_tmp[len++]='-';

    if (width > 0 && !leftadjust)
        for (int i = len ; i < width; i++)
            putc_iop(iop, ' ');

    for (int i = len-1; i >= 0; i--)
        putc_iop(iop, pprintf_tmp[i]);

    if (width > 0 && leftadjust)
        for (int i = len ; i < width; i++)
            putc_iop(iop, ' ');
}

static inline void pprintf_unsigned_decimal(iop_t *iop, int width, int leadingzero, int leftadjust, unsigned int v)
{
    char pprintf_tmp[64];
    int len = 0;

    do {
        int tv = v%10;
        v=v/10;
        pprintf_tmp[len++]=tv+'0';
    } while (v>0 || (leadingzero && len < width));

    if (width > 0 && !leftadjust)
        for (int i = len ; i < width; i++)
            putc_iop(iop, ' ');

    for (int i = len-1; i >= 0; i--)
        putc_iop(iop, pprintf_tmp[i]);

    if (width > 0 && leftadjust)
        for (int i = len ; i < width; i++)
            putc_iop(iop, ' ');
}

static inline void pprintf_hexidecimal(iop_t *iop, int width, int leadingzero, int leftadjust, uint32_t v)
{
    char pprintf_tmp[64];
    int len = 0;

    do {
        int tv = v&0x0f;
        v=v>>4;
        pprintf_tmp[len++]=tv < 10 ? tv+'0' : tv+'a'-10;
    } while (v>0 || (leadingzero && len < width));

    if (width > 0 && !leftadjust)
        for (int i = len ; i < width; i++)
            putc_iop(iop, ' ');

    for (int i = len-1; i >= 0; i--)
        putc_iop(iop,pprintf_tmp[i]);

    if (width > 0 && leftadjust)
        for (int i = len ; i < width; i++)
            putc_iop(iop, ' ');

}

static inline void pprintf_binary(iop_t *iop, int width, int leadingzero, int leftadjust, uint32_t v)
{
    char pprintf_tmp[64];
    int len = 0;

    do {
        int tv = v&0x01;
        v=v>>1;
        pprintf_tmp[len++]= tv+'0';
    } while (v>0 || (leadingzero && len < width));

    if (width > 0 && !leftadjust)
        for (int i = len ; i < width; i++)
            putc_iop(iop, ' ');

    for (int i = len-1; i >= 0; i--)
        putc_iop(iop, pprintf_tmp[i]);

    if (width > 0 && leftadjust)
        for (int i = len ; i < width; i++)
            putc_iop(iop, ' ');

}

static inline void pprintf_string(iop_t *iop, int width, int leftadjust, char *s)
{
    int len = strlen(s);

    if (width > 0 && !leftadjust)
        for (int i = len; i < width; i++)
            putc_iop(iop, ' ');

    for (int i = 0; i < len; i++)
        putc_iop(iop, s[i]);

    if (width > 0 && leftadjust)
        for (int i = len; i < width; i++)
            putc_iop(iop, ' ');
}

void vpprintf(iop_t *iop, const char *fmt, va_list ap)
{
    int leadingzero;
    int leftadjust;
    int precision;
    int width;

    while (*fmt)
    {
        switch(*fmt)
        {
        case '%':
            fmt++;

            // parse flag characters
            leadingzero = 0;
            leftadjust = 0;
            while (1)
            {
                if (*fmt=='0') {
                    leadingzero = 1;
                    fmt++;
                    continue;
                }
                if (*fmt=='-') {
                    leftadjust = 1;
                    fmt++;
                    continue;
                }
                break;
            }

            // consume a width formatter
            width = 0;
            while (1) {
                if (isdigit(*fmt)) {
                    width = width*10 + *fmt-'0';
                    fmt++;
                    continue;
                }
                break;
            }

            // consume precision
            precision = 6;
            if (*fmt=='.') {
                fmt++;
                precision = 0;
                while (1) {
                    if (isdigit(*fmt)) {
                        precision = precision*10 + *fmt-'0';
                        fmt++;
                        continue;
                    }
                    break;
                }
            }

            // consume length specifiers (we ignore them)
            while (*fmt=='l' || *fmt=='L')
                fmt++;

            // here's the format specifier, finally.
            switch (*fmt)
            {
            case 'b':
                pprintf_binary(iop, width, leadingzero, leftadjust, va_arg(ap, int));
                break;
            case 'i':
            case 'd':
                pprintf_decimal(iop, width, leadingzero, leftadjust, va_arg(ap, int));
                break;
            case 'u':
                pprintf_unsigned_decimal(iop, width, leadingzero, leftadjust, va_arg(ap, int));
                break;
            case 'x':
            case 'X':
            case 'p':
                pprintf_hexidecimal(iop, width, leadingzero, leftadjust, va_arg(ap, int32_t));
                break;
            case 's':
                pprintf_string(iop, width, leftadjust, va_arg(ap, char*));
                break;
            case 'c':
                putc_iop(iop, va_arg(ap, int));
                break;
            case '%':
                putc_iop(iop, '%');
                break;
            case 'f':
                pprintf_double(iop, width, leadingzero, leftadjust, precision, va_arg(ap, double));
                break;
            }
            fmt++;
            break;

        default:
            putc_iop(iop, *fmt);
            fmt++;
            break;
        }
    }
}

void pprintf(iop_t *iop, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    vpprintf(iop, fmt, ap);

    va_end(ap);
}

static int putc_write_wrapper(iop_t *iop, const void *buf, uint32_t len)
{
    void (*putc)(char c) = iop->user;

    for (uint32_t i = 0; i < len; i++)
        putc(((char*)buf)[i]);

    return len;
}

void pprintf_putc(void (*putc)(char c), const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    iop_t iop;
    iop.user = putc;
    iop.write = putc_write_wrapper;
    vpprintf(&iop, fmt, ap);

    va_end(ap);
}

