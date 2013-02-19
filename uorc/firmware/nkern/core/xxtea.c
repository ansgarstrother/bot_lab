
#include <stdint.h>
#include <stdlib.h>

/** XXTEA encryption algorithm, released into public domain by its
 * designers David Wheeler and Roger Needham.
 *
 * 
 **/

#define MX  ( (((z>>5)^(y<<2))+((y>>3)^(z<<4)))^((sum^y)+(key[(p&3)^e]^z)) )

void xxtea_encode(uint32_t *v, uint32_t length, uint32_t *key) 
{
    uint32_t z=v[length-1], y=v[0], sum=0, e, DELTA=0x9e3779b9;
    uint32_t p, q ;

    z=v[length-1];
    q = 6 + 52/length;
    while (q-- > 0) {
        sum += DELTA;
        e = (sum >> 2) & 3;
        for (p=0; p<length-1; p++) y = v[p+1], z = v[p] += MX;
        y = v[0];
        z = v[length-1] += MX;
    }
}

void xxtea_decode(uint32_t *v, uint32_t length, uint32_t *key)
{
    uint32_t z, y=v[0], sum=0, e, DELTA=0x9e3779b9;
    uint32_t p, q ;

    q = 6 + 52/length;
    sum = q*DELTA ;
    while (sum != 0) {
        e = (sum >> 2) & 3;
        for (p=length-1; p>0; p--) z = v[p-1], y = v[p] -= MX;
        z = v[length-1];
        y = v[0] -= MX;
        sum -= DELTA;
    }
}

////////////////////////////////////////////////////////////////////////////
// Test code beneath here.
#if 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// test vectors

// convert hex digit in ascii to integer value.
uint32_t c2int(char c)
{
    if (c>='A' && c<='F')
        return c-'A'+10;
    if (c>='a' && c<='f')
        return c-'a'+10;
    if (c>='0' && c<='9')
        return c-'0';
    exit(-1);
}

// converts a hexidecimal string to uint32_t*. returns # of bytes.
uint32_t string2array(const char *s, uint32_t *v)
{
    int inpos = 0;
    int outpos = 0;

    while (s[inpos]!=0) {
        uint32_t v0 = c2int(s[inpos++])*16 + c2int(s[inpos++]);
        uint32_t v1 = c2int(s[inpos++])*16 + c2int(s[inpos++]);
        uint32_t v2 = c2int(s[inpos++])*16 + c2int(s[inpos++]);
        uint32_t v3 = c2int(s[inpos++])*16 + c2int(s[inpos++]);
     
// You might need to change this based on your systems' endian-ness?
//        int v0123 = (v0<<24) + (v1<<16) + (v2<<8) + v3;
        int v0123 = (v3<<24) + (v2<<16) + (v1<<8) + v0;

        v[outpos++] = v0123;
    }
    return outpos*4;
}

void print(const uint32_t *v, uint32_t bytelen)
{
    for (int i = 0; i < bytelen/4; i++)
        printf("%08x", v[i]);
    printf("\n");
}

// test a single test vector.
void testvec(const char *keys, const char *plaintexts, const char *ciphertexts)
{
    uint32_t key[4];
    uint32_t plaintext[1024];
    uint32_t ciphertext[1024];
    uint32_t tmp[1024];

    int key_length        = string2array(keys, key);
    int plaintext_length  = string2array(plaintexts, plaintext);
    int ciphertext_length = string2array(ciphertexts, ciphertext);

    if (key_length != 16 || plaintext_length != ciphertext_length) {
        printf("BAD LENGTHS\n");
        exit(0);
    }
     
    memcpy(tmp, plaintext, plaintext_length);

    print(tmp, plaintext_length);
    xxtea_encode(tmp, plaintext_length/4, key);
    print(tmp, plaintext_length);
    print(ciphertext, plaintext_length);

    if (memcmp(tmp, ciphertext, plaintext_length)) {
        printf("Encode error\n");
    }

    xxtea_decode(tmp, plaintext_length/4, key);
    if (memcmp(tmp, plaintext, plaintext_length)) {
        printf("Decode error\n");
    }
}

int main(int argc, char *argv[])
{
    //                           key (128b)                          plaintext (64b)     ciphertext (64b)
    const char* testvecs[][3] = {{"00000000000000000000000000000000", "0000000000000000", "ab043705808c5d57"},
                                 {"0102040810204080fffefcf8f0e0c080", "0000000000000000", "d1e78be2c746728a"},
                                 {"9e3779b99b9773e9b979379e6b695156", "ffffffffffffffff", "67ed0ea8e8973fc5"},
                                 {"0102040810204080fffefcf8f0e0c080", "fffefcf8f0e0c080", "8c3707c01c7fccc4"},
                                 { NULL,                               NULL,               NULL}};

    
    int idx = 0;
    while (testvecs[idx][0] != NULL) {
        testvec(testvecs[idx][0], testvecs[idx][1], testvecs[idx][2]);
        idx++;
    }

    printf("PASSED\n");
}

#endif
