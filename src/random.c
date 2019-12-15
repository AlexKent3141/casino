#ifndef __CASINO_RANDOM_INCLUDED_H__
#define __CASINO_RANDOM_INCLUDED_H__

#include "../include/casino.h"
#include "casino_state.h"

uint64_t xorshift128plus(uint64_t s[2])
{
    uint64_t x = s[0];
    uint64_t y = s[1];
    s[0] = y;
    x ^= x << 23;
    s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
    return s[1] + y;
}

int CAS_Random(void* st, int bound)
{
    struct CAS_State* cas = (struct CAS_State*)st;
    return xorshift128plus(cas->prngState) % bound;
}

#endif /* __CASINO_RANDOM_INCLUDED_H__ */
