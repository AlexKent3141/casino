#ifndef __CASINO_STATE_INCLUDED_H__
#define __CASINO_STATE_INCLUDED_H__

#include "../include/casino.h"
#include "memory.h"

typedef struct PRNGState { uint64_t x[2]; } PRNGState;

struct CAS_State
{
    struct CAS_Domain* domain;
    struct CAS_Node* root;
    int maxActions;
    struct MemoryState* mem;
};

uint64_t Random(void*);

struct CAS_ActionList* GetActionList(void* st);

#endif /* __CASINO_STATE_INCLUDED_H__ */
