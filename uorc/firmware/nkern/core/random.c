#include <stdint.h>
#include <string.h>

#include "md5.h"

static uint32_t fast_entropy_bits;
static uint64_t fast_entropy_pool;

static uint8_t  md5_entropy_pool[8];
static uint32_t md5_entropy_bits;

static uint32_t stir_counter;
static uint32_t take_counter;

/** entropy is a number containing some entropy. nbits is an estimate
 * of that entropy. This function is fairly fast (it does not stir the
 * pool.)
 **/
void nkern_random_entropy_add(uint32_t entropy, uint32_t nbits)
{
    fast_entropy_bits += nbits;
    if (fast_entropy_bits > 64)
        fast_entropy_bits = 64;

    // stir entropy into pool using a rotation of a prime # of bits
    // (to maximize period)
    fast_entropy_pool = (fast_entropy_pool << 23) + (fast_entropy_pool >> 41) + entropy;
}

uint32_t nkern_random_bits_available()
{
    uint32_t t = md5_entropy_bits + fast_entropy_bits;
    if (t > 64)
        t = 64;

    return t;
}

void nkern_random_stir()
{
    MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, md5_entropy_pool, 8);
    MD5Update(&ctx, (uint8_t*) &fast_entropy_pool, 8);
    MD5Update(&ctx, (uint8_t*) &stir_counter, 4);
    MD5Final(&ctx);

    memcpy(md5_entropy_pool, ctx.digest, 8);

    stir_counter++;

    md5_entropy_bits = md5_entropy_bits + fast_entropy_bits;
    if (md5_entropy_bits > 64)
        md5_entropy_bits = 64;
    fast_entropy_bits = 0;
}

uint32_t nkern_random_take(uint32_t nbits)
{
    if (fast_entropy_bits > 0)
        nkern_random_stir();

    MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, md5_entropy_pool, 8);
    MD5Update(&ctx, (uint8_t*) &fast_entropy_pool, 8);
    MD5Update(&ctx, (uint8_t*) &take_counter, 4);
    MD5Final(&ctx);

    take_counter++;

    uint32_t v = (ctx.digest[0]<<24) + (ctx.digest[1]<<16) + 
        (ctx.digest[2]<<8) + ctx.digest[3];

    if (nbits < 32)
        v = v & ((1<<nbits)-1);

    if (md5_entropy_bits < nbits)
        md5_entropy_bits = 0;
    else
        md5_entropy_bits -= nbits;

    return v;
}

