#include "node.h"
#include "memory.h"
#include "assert.h"

struct CAS_Node* MakeRoot(struct MemoryState* mem, enum CAS_Player player)
{
    struct CAS_Node* root;

    mem->bufRoot = mem->bufNext;

    root = (struct CAS_Node*)GetMemory(mem, sizeof(struct CAS_Node));
    root->parent = NULL;
    root->player = player;
    root->action = CAS_BAD_ACTION;
    root->stage = 0;
    root->expanded = false;
    root->children = NULL;
    root->wins = 0;
    root->draws = 0;
    root->playouts = 0;

    return root;
}

struct CAS_NodeList* GetNodeList(struct MemoryState* mem, size_t maxActions)
{
    struct CAS_NodeList* list;

    list = (struct CAS_NodeList*)GetMemory(mem, sizeof(struct CAS_NodeList));
    if (list == NULL)
        return NULL;

    list->nodes =
        (struct CAS_Node*)GetMemory(mem, maxActions*sizeof(struct CAS_Node));
    if (list->nodes == NULL)
        return NULL;

    list->numNodes = 0;

    return list;
}

/* This method creates and appends a new node. */
void AddNode(struct CAS_NodeList* list,
             struct CAS_Node* parent,
             enum CAS_Player nextPlayer,
             int nextActionStage,
             CAS_Action action)
{
    struct CAS_Node* n;
    
    assert(parent != NULL);
    assert(parent->player != CAS_NONE);
    assert(list != NULL);
    assert(list->nodes != NULL);

    n = &list->nodes[list->numNodes++];
    n->parent = parent;
    n->player = nextPlayer;
    n->action = action;
    n->stage = nextActionStage;
    n->expanded = false;
    n->children = NULL; /* This is initialised during expansion. */
    n->wins = 0;
    n->draws = 0;
    n->playouts = 0;
}
