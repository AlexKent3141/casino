#define _DEFAULT_SOURCE

#include "../include/casino.h"
#include "casino_state.h"
#include "mcts.h"
#include "memory.h"
#include "ropemaker.h"

void* CAS_Init(
    struct CAS_Domain* domain,
    size_t bufSize,
    char* buf)
{
    struct MemoryState* mem;
    struct CAS_State* cas;

    /* Initialise memory management. */
    mem = MemoryInit(bufSize, buf);
    if (mem == NULL)
        return NULL;

    /* Initialise the internal state. */
    cas = (struct CAS_State*)GetMemory(
        mem,
        sizeof(struct CAS_State));

    if (cas == NULL)
        return NULL;

    cas->domain = domain;
    cas->maxActions = domain->maxActionsPerTurn;
    cas->root = NULL;
    cas->mem = mem;

    return (void*)cas;
}

struct CAS_Node* GetChildWithMostPlayouts(
    struct CAS_Node* parent)
{
    struct CAS_Node* bestNode = NULL, *currentNode;
    int mostPlayouts = 0;
    size_t i;

    for (i = 0; i < parent->children->numNodes; i++)
    {
        currentNode = &parent->children->nodes[i];
        if (currentNode->playouts > mostPlayouts)
        {
            mostPlayouts = currentNode->playouts;
            bestNode = currentNode;
        }
    }

    return bestNode;
}

/* Parse the tree to find the best action. */
void CAS_GetBestAction(
    void* state,
    struct CAS_ActionStats* stats)
{
    struct CAS_State* cas = (struct CAS_State*)state;
    struct CAS_Node* bestNode = GetChildWithMostPlayouts(cas->root);

    if (bestNode != NULL)
    {
        stats->action = bestNode->action;
        stats->winRate = CAS_WinRate(bestNode);
        stats->playouts = bestNode->playouts;;
    }
}

/* Parse the tree to find the PV up to maximum depth `len`. */
int CAS_GetPV(
    void* state,
    int len, CAS_Action* buf)
{
    struct CAS_State* cas = (struct CAS_State*)state;
    struct CAS_Node* bestNode = GetChildWithMostPlayouts(cas->root);
    int i = 0;

    while (bestNode != NULL && bestNode->children != NULL && i < len)
    {
        buf[i++] = bestNode->action;
        bestNode = GetChildWithMostPlayouts(bestNode);
    }

    return i;
}

enum CAS_SearchResult CAS_Search(
    void* state,
    struct CAS_SearchConfig* config,
    CAS_DomainState initialPosition,
    CAS_DomainState* workerPositions,
    enum CAS_Player player,
    int duration)
{
    struct CAS_State* cas;
    struct WorkerData* data;
    PRNGState* prngStates;
    rmk_thread_t* tids;
    rmk_mutex_t treeLock;
    size_t i;

    PRNGState prngInitial =
    {
        {
            0xdeadbeefcafebabe,
            0x8badf00dbaada555
        }
    };

    cas = (struct CAS_State*)state;
    if (cas == NULL)
        return CAS_INSUFFICIENT_MEMORY;

    ResetTree(cas->mem);

    /* Initialise a mutex to protect access to the tree. */
    if (!rmk_mutex_create(&treeLock))
        return CAS_COULD_NOT_INITIALISE_MUTEX;

    /* Initialise the root node. */
    /* This also flags to the memory state where to start in the buffer. */
    cas->root = MakeRoot(cas->mem, player);
    if (cas->root == NULL)
        return CAS_INSUFFICIENT_MEMORY;

    /* Initialise and kick off each of the worker threads. */
    tids = (rmk_thread_t*)GetMemory(
        cas->mem,
        config->numThreads*sizeof(rmk_thread_t));

    if (tids == NULL)
        return CAS_INSUFFICIENT_MEMORY;

    /* Each worker must have its own PRNG state. */
    prngStates = (PRNGState*)GetMemory(
        cas->mem,
        config->numThreads*sizeof(struct PRNGState));

    if (prngStates == NULL)
        return CAS_INSUFFICIENT_MEMORY;

    for (i = 0; i < config->numThreads; i++)
    {
        prngStates[i].x[0] = Random(&prngInitial);
        prngStates[i].x[1] = Random(&prngInitial);

        data = (struct WorkerData*)GetMemory(
            cas->mem,
            sizeof(struct WorkerData));

        data->cas = cas;
        data->config = config;
        data->initialPosition = initialPosition;
        data->treeLock = &treeLock;

        /* Each worker needs its own action list to populate and domain state
           to work from. */
        data->actionList = GetActionList(cas);
        if (data->actionList == NULL)
            return CAS_INSUFFICIENT_MEMORY;

        data->workerPosition = workerPositions[i];
        data->prngState = &prngStates[i];

        rmk_thread_create(&tids[i], RMK_JOINABLE, &SearchWorker, data);
    }

    rmk_sleep_ms(duration);

    /* Wait for all threads to terminate. */
    for (i = 0; i < config->numThreads; i++)
    {
        rmk_thread_request_stop(tids[i]);
        rmk_thread_join(tids[i]);
    }

    rmk_mutex_destroy(&treeLock);

    return CAS_SUCCESS;
}
