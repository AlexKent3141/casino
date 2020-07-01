#ifndef __CASINO_NODE_INCLUDED_H__
#define __CASINO_NODE_INCLUDED_H__

#include "../include/casino.h"
#include "memory.h"

struct CAS_Node* MakeRoot(
    struct MemoryState* mem,
    enum CAS_Player player);

/* Methods for creating and interacting with node lists. */
struct CAS_NodeList* GetNodeList(
    struct MemoryState* mem,
    size_t maxActions);

void AddNode(
    struct CAS_NodeList* list,
    struct CAS_Node* parent,
    enum CAS_Player nextPlayer,
    int nextActionStage,
    CAS_Action action);

bool FullyExpanded(
    struct CAS_Node* n);

#endif /* __CASINO_NODE_INCLUDED_H__ */
