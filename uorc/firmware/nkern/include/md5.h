#ifndef _MD5_H
#define _MD5_H

/* Data structure for MD5 (Message Digest) computation */
typedef struct {
    uint32_t i[2];                   /* number of _bits_ handled mod 2^64 */
    uint32_t buf[4];                                    /* scratch buffer */
    unsigned char in[64];                              /* input buffer */
    unsigned char digest[16];     /* actual digest after MD5Final call */
} MD5_CTX;

void MD5Init(MD5_CTX *mdContext);
void MD5Update(MD5_CTX *mdContext, uint8_t *inBuf, uint32_t inLen);
void MD5Final(MD5_CTX *mdContext);

#endif
