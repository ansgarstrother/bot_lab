#ifndef _FLASH_H
#define _FLASH_H

void flash_erase(uint32_t addr1k);
void flash_write(uint32_t addr, uint32_t value);
void flash_erase_and_write(uint32_t addr, void *_buf, uint32_t _len);

#endif
