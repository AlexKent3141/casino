#include "../include/casino.h"
#include "casino_state.h"
#include "memory.h"
#include "node.h"
#include "time.h"

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
            nextPlayer = nextPlayer == P1 ? P2 : P1;

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
    while (action != BAD_ACTION)
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
            if (winner == NONE)
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

enum CAS_SearchResult CAS_Search(void* state,   
                                 struct CAS_SearchConfig* config,
                                 CAS_DomainState initialPosition,
                                 enum CAS_Player player,
                                 int ms)
{
    struct CAS_State* cas;
    struct CAS_Domain* domain;
    struct CAS_ActionList* actionList;
    CAS_DomainState pos;
    enum CAS_Player winner;
    struct CAS_Node* n, *selected;
    clock_t startTime;

    cas = (struct CAS_State*)state;
    if (cas == NULL)
        return INSUFFICIENT_MEMORY;

    ResetTree(cas->mem);

    /* Initialise the root node. */
    /* This also flags to the memory state where to start in the buffer. */
    cas->root = MakeRoot(cas->mem, player);
    if (cas->root == NULL)
        return INSUFFICIENT_MEMORY;

    /* Perform the MCTS procedure while resources remain. */
    domain = cas->domain;
    actionList = GetActionList(cas);
    startTime = clock();
    pos = GetMemory(cas->mem, domain->domainStateSize);
    if (pos == NULL)
        return INSUFFICIENT_MEMORY;

    do 
    {
        domain->CopyState(initialPosition, pos);

        selected = Select(cas, config, domain, cas->root, pos);
        n = Expand(cas, domain, selected, pos, actionList);
        if (n == NULL)
        {
            /* Out of memory, just keep doing playouts on the current tree. */
            n = selected;
        }

        winner = Simulate(cas, config, domain, pos, actionList);
        Backprop(n, winner);

    } while (TimeSinceStart(startTime) < ms/1000.0);

    /* Now examine the tree to find the best move. */
    return SUCCESS;
}
