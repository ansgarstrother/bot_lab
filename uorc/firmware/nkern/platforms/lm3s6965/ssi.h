#ifndef _SSI_H
#define _SSI_H

#include <nkern.h>
#include <nkern_sync.h>

void ssi_init();
void ssi_lock();
void ssi_unlock();
void ssi_config(uint32_t maxclk, int spo, int sph, int nbits);
int ssi_rxtx(const uint32_t *tx, uint32_t *rx, uint32_t nwords);

#endif
