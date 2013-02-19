#ifndef _CHARLCD_H
#define _CHARLCD_H

void charlcd_init();

void charlcd_clear();
void charlcd_goto(int x, int y);
void charlcd_on();
void charlcd_cursor_off();
void charlcd_putc( char c ) ;
void charlcd_puts(const char *s);
long charlcd_write_fd(void *user, const void *data, int len);

#endif
