#include "node.h"
#include "memory.h"
#include "assert.h"

struct Node* MakeRoot(struct CAS_State* cas, enum CAS_Player player)
{
    struct Node* root;

    root = (struct Node*)GetMemory(cas->mem, sizeof(struct Node));
    root->parent = NULL;
    root->player = player;
    root->action = BAD_ACTION;
    root->expanded = false;
    root->children = NULL;
    root->wins = 0;
    root->draws = 0;
    root->playouts = 0;

    cas->root = root;

    return root;
}

struct NodeList* GetNodeList(struct CAS_State* cas)
{
    struct NodeList* list;

    list = (struct NodeList*)GetMemory(cas->mem, sizeof(struct NodeList));
    if (list == NULL)
        return NULL;

    list->nodes =
        (struct Node*)GetMemory(cas->mem, cas->maxActions*sizeof(struct Node));
    if (list->nodes == NULL)
        return NULL;

    list->numNodes = 0;

    return list;
}

/* This method creates and appends a new node. */
void AddNode(struct NodeList* list, struct Node* parent, CAS_Action action)
{
    struct Node* n;
    
    assert(parent != NULL);
    assert(parent->player != NONE);
    assert(list != NULL);
    assert(list->nodes != NULL);

    n = &list->nodes[list->numNodes++];
    n->parent = parent;
    n->player = parent->player == P1 ? P2 : P1;
    n->action = action;
    n->expanded = false;
    n->children = NULL; /* This is initialised during expansion. */
    n->wins = 0;
    n->draws = 0;
    n->playouts = 0;
}
