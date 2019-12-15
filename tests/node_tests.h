#ifndef __CASINO_NODE_TESTS_INCLUDED_H__
#define __CASINO_NODE_TESTS_INCLUDED_H__ 

#include "greatest/greatest.h"
#include "../src/memory.h"
#include "../src/node.h"
#include "stdlib.h"

char* buf = NULL;

/*
 * Get a memory state which can allocate the specified number of bytes (in addition
 * to its own internal state).
 */
struct MemoryState* GetMemoryState(size_t testBytes)
{
    size_t requiredBytes;

    if (buf != NULL)
        free(buf);

    requiredBytes = sizeof(struct MemoryState) + testBytes;
    buf = malloc(requiredBytes);

    return MemoryInit(requiredBytes, buf);
}

TEST MakeRoot_Test()
{
    struct MemoryState* mem;
    struct CAS_Node* root;

    /* Create a root Node. */
    mem = GetMemoryState(sizeof(struct CAS_Node));
    root = MakeRoot(mem, P1);
    ASSERT(root != NULL);

    free(buf);
    buf = NULL;

    PASS();
}

TEST GetNodeList_Test()
{
    const int MaxActions = 10;
    struct MemoryState* mem;
    struct CAS_NodeList* nodeList;

    /* Create a MemoryState and a NodeList. */
    mem = GetMemoryState(sizeof(struct CAS_NodeList) + MaxActions*sizeof(struct CAS_Node));
    nodeList = GetNodeList(mem, MaxActions);
    ASSERT(nodeList != NULL);

    free(buf);
    buf = NULL;

    PASS();
}

TEST AddNode_Test()
{
    const size_t MaxActions = 10;
    struct MemoryState* mem;
    struct CAS_NodeList* nodeList;
    struct CAS_Node* root;
    size_t action;

    /* Create a MemoryState and a root Node. */
    mem = GetMemoryState(sizeof(struct CAS_NodeList)
        + (MaxActions+1)*sizeof(struct CAS_Node));
    root = MakeRoot(mem, P1);
    ASSERT(root != NULL);

    /* Create a NodeList. */
    nodeList = GetNodeList(mem, MaxActions);
    ASSERT(nodeList != NULL);

    /* Add nodes. */
    for (action = 0; action < MaxActions; action++)
    {
        AddNode(nodeList, root, action);
    }

    ASSERT(nodeList->numNodes == MaxActions);

    free(buf);
    buf = NULL;

    PASS();
}

SUITE(NodeTests)
{
    RUN_TEST(MakeRoot_Test);
    RUN_TEST(GetNodeList_Test);
    RUN_TEST(AddNode_Test);
}

#endif /* __CASINO_NODE_TESTS_INCLUDED_H__ */
