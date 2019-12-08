#ifndef __CASINO_NODE_INCLUDED_H__
#define __CASINO_NODE_INCLUDED_H__

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

struct Node* MakeRoot(struct MemoryState* mem, enum CAS_Player player);

/* Methods for creating and interacting with node lists. */
struct NodeList* GetNodeList(struct MemoryState* mem, size_t maxActions);
void AddNode(struct NodeList* list, struct Node* parent, CAS_Action action);

#endif /* __CASINO_NODE_INCLUDED_H__ */
