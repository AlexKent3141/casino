#ifndef __CASINO_NODE_INCLUDED_H__
#define __CASINO_NODE_INCLUDED_H__

#include "../include/casino.h"
#include "casino_state.h"

struct Node* MakeRoot(struct CAS_State* cas, enum CAS_Player player);

/* Methods for creating and interacting with node lists. */
struct NodeList* GetNodeList(struct CAS_State* cas);
void AddNode(struct NodeList* list, struct Node* parent, CAS_Action action);

#endif /* __CASINO_NODE_INCLUDED_H__ */
