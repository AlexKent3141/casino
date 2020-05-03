#include "../include/casino.h"
#include "casino_state.h"
#include "memory.h"
#include "node.h"
#include "time.h"
#include "thread.h"

void* CAS_Init(struct CAS_Domain* domain, size_t bufSize, char* buf)
{
    struct MemoryState* mem;
    struct CAS_State* cas;

    /* Initialise memory management. */
    mem = MemoryInit(bufSize, buf);
    if (mem == NULL)
        return NULL;

    /* Initialise the internal state. */
    cas = (struct CAS_State*)GetMemory(mem, sizeof(struct CAS_State));
    if (cas == NULL)
        return NULL;

    cas->domain = domain;
    cas->maxActions = domain->maxActionsPerTurn;
    cas->root = NULL;
    cas->mem = mem;

    /* Seed the PRNG. */
    cas->prngState[0] = 0xdeadbeefcafebabe;
    cas->prngState[1] = 0x8badf00dbaada555;

    return (void*)cas;
}

/* Keep selecting child nodes until a leaf node (or a terminal node) is reached. */
struct CAS_Node* Select(void* cas,
                        struct CAS_SearchConfig* config,
                        struct CAS_Domain* domain,
                        struct CAS_Node* n,
                        CAS_DomainState position)
{
    struct CAS_Node* selected = n;

    while (selected->expanded
        && selected->children != NULL
        && selected->children->numNodes > 0)
    {
        selected = config->SelectionPolicy(cas, position, selected);
        domain->DoAction(position, selected->action);
    }

    return selected;
}

/* Add nodes for each of the actions available from n and pick the first. */
struct CAS_Node* Expand(struct CAS_State* cas,
                        struct CAS_Domain* domain,
                        struct CAS_Node* n,
                        CAS_DomainState position,
                        struct CAS_ActionList* actionList)
{
    struct CAS_Node* expanded = n;
    enum CAS_Player nextPlayer;
    int nextActionStage;
    size_t i;

    actionList->numActions = 0;
    domain->GetStateActions(position, actionList);
    if (actionList->numActions > 0)
    {
        /* Work out who the next player is taking into account action stages. */
        nextPlayer = n->player;
        nextActionStage = (n->stage + 1) % domain->actionStages;
        if (nextActionStage == 0)
            nextPlayer = nextPlayer == CAS_P1 ? CAS_P2 : CAS_P1;

        n->children = GetNodeList(cas->mem, actionList->numActions);
        if (n->children == NULL)
            return NULL;

        for (i = 0; i < actionList->numActions; i++)
        {
            AddNode(n->children,
                    n,
                    nextPlayer,
                    nextActionStage,
                    actionList->actions[i]);
        }

        expanded = &n->children->nodes[0];
    }

    n->expanded = true;
    return expanded;
}

/* Play out a random game from the point and score the terminal state. */
enum CAS_Player Simulate(struct CAS_State* cas,
                         struct CAS_SearchConfig* config,
                         struct CAS_Domain* domain,
                         CAS_DomainState position,
                         struct CAS_ActionList* actionList)
{
    CAS_Action action;

    actionList->numActions = 0;
    action = config->PlayoutPolicy(cas, domain, position, actionList);
    while (action != CAS_BAD_ACTION)
    {
        domain->DoAction(position, action);

        /* Select which move to make next using the playout policy. */
        actionList->numActions = 0;
        action = config->PlayoutPolicy(cas, domain, position, actionList);
    }

    return domain->GetScore(position);
}

void Backprop(struct CAS_Node* n, enum CAS_Player winner)
{
    do
    {
        /* A node's score is a reflection of how good the move to reach that node
           was i.e. a move for the other player. This is why we check against the
           parent node's player below. */
        n->playouts++;
        if (n->parent != NULL)
        {
            if (winner == CAS_NONE)
                n->draws++;
            else if (n->parent->player == winner)
                n->wins++;
        }

        n = n->parent;

    } while (n != NULL);
}

struct CAS_Node* GetChildWithMostPlayouts(struct CAS_Node* parent)
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
void CAS_GetBestAction(void* state, struct CAS_ActionStats* stats)
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
int CAS_GetPV(void* state, int len, CAS_Action* buf)
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

double TimeSinceStart(clock_t start)
{
    return (double)(clock() - start) / CLOCKS_PER_SEC;
}

struct WorkerData
{
    struct CAS_State* cas;
    struct CAS_SearchConfig* config;
    CAS_DomainState initialPosition;
    int duration;
    mutex_t* treeLock;
    struct CAS_ActionList* actionList;
    CAS_DomainState workerPosition;
};

void* SearchWorker(void* data)
{
    struct WorkerData* workerData = (struct WorkerData*)data;
    struct CAS_Node* n, *selected;
    enum CAS_Player winner;
    struct CAS_State* cas = workerData->cas;
    struct CAS_Domain* domain = cas->domain;
    clock_t startTime = clock();

    /* Perform the MCTS procedure while resources remain. */
    /* All access to the tree is synchronised by the shared mutex. */
    do
    {
        domain->CopyState(
            workerData->initialPosition,
            workerData->workerPosition);

        selected = Select(
            cas,
            workerData->config,
            cas->domain,
            cas->root,
            workerData->workerPosition);

        LockMutex(workerData->treeLock);

        n = Expand(
            cas,
            cas->domain,
            selected,
            workerData->workerPosition,
            workerData->actionList);

        UnlockMutex(workerData->treeLock);

        if (n == NULL)
        {
            /* Out of memory, just keep doing playouts on the current tree. */
            n = selected;
        }

        winner = Simulate(
            cas,
            workerData->config,
            cas->domain,
            workerData->workerPosition,
            workerData->actionList);

        LockMutex(workerData->treeLock);

        Backprop(n, winner);

        UnlockMutex(workerData->treeLock);

    } while (TimeSinceStart(startTime) < workerData->duration / 1000.0);

    return NULL;
}

enum CAS_SearchResult CAS_Search(void* state,   
                                 struct CAS_SearchConfig* config,
                                 CAS_DomainState initialPosition,
                                 enum CAS_Player player,
                                 int duration)
{
    struct CAS_State* cas;
    struct WorkerData* workerData;
    thread_t* tids;
    size_t i;
    mutex_t treeLock;

    if (CreateMutex(&treeLock) != 0)
        return CAS_COULD_NOT_INITIALISE_MUTEX;

    cas = (struct CAS_State*)state;
    if (cas == NULL)
        return CAS_INSUFFICIENT_MEMORY;

    ResetTree(cas->mem);

    /* Initialise the root node. */
    /* This also flags to the memory state where to start in the buffer. */
    cas->root = MakeRoot(cas->mem, player);
    if (cas->root == NULL)
        return CAS_INSUFFICIENT_MEMORY;

    /* Initialise and kick off each of the worker threads. */
    tids = (thread_t*)GetMemory(
        cas->mem,
        config->numThreads*sizeof(thread_t));

    for (i = 0; i < config->numThreads; i++)
    {
        workerData = (struct WorkerData*)GetMemory(
            cas->mem,
            sizeof(struct WorkerData));

        workerData->cas = cas;
        workerData->config = config;
        workerData->initialPosition = initialPosition;
        workerData->duration = duration,
        workerData->treeLock = &treeLock,

        /* Each worker needs its own action list to populate and domain state
           to work from. */
        workerData->actionList = GetActionList(cas);
        workerData->workerPosition =
            GetMemory(cas->mem, cas->domain->domainStateSize);

        CreateThread(&tids[i], &SearchWorker, workerData);
    }

    /* Wait for all threads to terminate. */
    for (i = 0; i < config->numThreads; i++)
        JoinThread(&tids[i]);

    return CAS_SUCCESS;
}
