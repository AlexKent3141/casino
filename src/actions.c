#include "../include/casino.h"
#include "casino_state.h"
#include "memory.h"
#include "stdlib.h"
#include "assert.h"

struct CAS_ActionList* GetActionList(
    void* st)
{
    struct CAS_State* cas = (struct CAS_State*)st;

    struct CAS_ActionList* list = (struct CAS_ActionList*)GetMemory(
        cas->mem,
        sizeof(struct CAS_ActionList));

    if (list == NULL)
        return NULL;

    list->actions = (CAS_Action*)GetMemory(
        cas->mem,
        cas->maxActions*sizeof(CAS_Action));

    if (list->actions == NULL)
        return NULL;

    list->numActions = 0;

    return list;
}

void CAS_AddAction(
    struct CAS_ActionList* list,
    CAS_Action action)
{
    assert(list != NULL);
    assert(list->actions != NULL);
    list->actions[list->numActions++] = action;
}
