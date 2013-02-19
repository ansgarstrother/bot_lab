#ifndef _USB_CDC_H
#define _USB_CDC_H

void usb_cdc_init();
int  usb_cdc_read(void *p);
void usb_cdc_write(const void *p, int len);
long usb_cdc_write_fd(void *user, const void *data, int len);

#endif
