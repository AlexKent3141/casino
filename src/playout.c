#ifndef __CASINO_PLAYOUT_INCLUDED_H__
#define __CASINO_PLAYOUT_INCLUDED_H__

#include "../include/casino.h"
#include "assert.h"

/* This is the uniform selection policy. */
CAS_Action CAS_DefaultPlayoutPolicy(void* cas,
                                    struct CAS_Domain* domain,
                                    CAS_DomainState position,
                                    struct CAS_ActionList* list)
{
    CAS_Action action = CAS_BAD_ACTION;
    domain->GetStateActions(position, list);
    if (list->numActions > 0)
    {
        action = list->actions[CAS_Random(cas, list->numActions)];
    }

    return action;
}

#endif /* __CASINO_PLAYOUT_INCLUDED_H__ */
