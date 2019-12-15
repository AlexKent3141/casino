#ifndef __CASINO_STATE_INCLUDED_H__
#define __CASINO_STATE_INCLUDED_H__

#include "../include/casino.h"
#include "memory.h"

struct CAS_State
{
    struct CAS_Domain* domain;
    struct CAS_Node* root;
    int maxActions;
    struct MemoryState* mem;
    uint64_t prngState[2];
};

struct CAS_ActionList* GetActionList(void* st);

#endif /* __CASINO_STATE_INCLUDED_H__ */
