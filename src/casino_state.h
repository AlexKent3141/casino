#ifndef __CASINO_STATE_INCLUDED_H__
#define __CASINO_STATE_INCLUDED_H__

#include "../include/casino.h"
#include "memory.h"

struct NodeList
{
    struct Node* nodes;
    size_t numNodes;
};

struct Node
{
    struct Node* parent;       /* The parent of this node. */
    enum CAS_Player player;    /* The player to move. */
    CAS_Action action;         /* The action that was applied to reach this node. */
    bool expanded;             /* Whether this node has been expanded. */
    struct NodeList* children; /* The child nodes of this node. */
    int wins;                  /* The number of wins during playouts. */
    int draws;                 /* The number of draws during playouts. */
    int playouts;              /* The total number of playouts. */
};

struct CAS_State
{
    struct CAS_Domain* domain;
    struct Node* root;
    int maxActions;
    struct MemoryState* mem;
};

struct CAS_ActionList* GetActionList(void* st);

#endif /* __CASINO_STATE_INCLUDED_H__ */
