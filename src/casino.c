#include "../include/casino.h"
#include "casino_state.h"
#include "math.h"
#include "memory.h"
#include "node.h"
#include "stdlib.h"
#include "stdint.h"
#include "stdio.h"
#include "time.h"
#include "assert.h"

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

    return (void*)cas;
}

uint64_t xorshift128plus(uint64_t s[2])
{
    uint64_t x = s[0];
    uint64_t y = s[1];
    s[0] = y;
    x ^= x << 23;
    s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
    return s[1] + y;
}

double ScoreUCB(struct Node* n)
{
    const double C = sqrt(2);
    double exploit = (n->wins + 0.5*n->draws)/n->playouts;
    double expand = C*sqrt(log(n->parent->playouts)/n->playouts);
    return exploit + expand;
}

/*
 * Select the best child using the UCB criterion.
 * If a child hasn't been visited yet then it should be selected next.
 */
struct Node* SelectChild(struct Node* n)
{
    struct Node* selected, *current;
    double score, bestScore;
    size_t i;

    bestScore = 0;
    for (i = 0; i < n->children->numNodes; i++)
    {
        current = &n->children->nodes[i];

        if (current->playouts == 0)
            return current;

        score = ScoreUCB(current);
        if (score >= bestScore)
        {
            selected = current;
            bestScore = score;
        }
    }

    return selected;
}

/* Keep selecting child nodes until a leaf node (or a terminal node) is reached. */
struct Node* Select(struct CAS_Domain* domain,
                    struct Node* n,
                    CAS_DomainState position)
{
    struct Node* selected;

    selected = n;
    while (selected->expanded
        && selected->children != NULL
        && selected->children->numNodes > 0)
    {
        selected = SelectChild(selected);
        domain->DoAction(position, selected->action);
    }

    return selected;
}

/* Add nodes for each of the actions available from n and pick the first. */
struct Node* Expand(struct CAS_State* cas,
                    struct CAS_Domain* domain,
                    struct Node* n,
                    CAS_DomainState position,
                    struct CAS_ActionList* actionList)
{
    struct Node* expanded;
    size_t i;

    expanded = n;
    actionList->numActions = 0;
    domain->GetStateActions(position, actionList);
    if (actionList->numActions > 0)
    {
        n->children = GetNodeList(cas->mem, cas->maxActions);
        if (n->children == NULL)
            return NULL;

        for (i = 0; i < actionList->numActions; i++)
        {
            AddNode(n->children, n, actionList->actions[i]);
        }

        expanded = &n->children->nodes[0];
    }

    n->expanded = true;
    return expanded;
}

/* Play out a random game from the point and score the terminal state. */
enum CAS_Player Simulate(struct CAS_Domain* domain,
                         CAS_DomainState position,
                         struct CAS_ActionList* actionList,
                         uint64_t* prngState)
{
    int i;

    actionList->numActions = 0;
    domain->GetStateActions(position, actionList);
    while (actionList->numActions > 0)
    {
        /* Currently just select moves uniformly. */
        i = xorshift128plus(prngState) % actionList->numActions;
        domain->DoAction(position, actionList->actions[i]);

        actionList->numActions = 0;
        domain->GetStateActions(position, actionList);
    }

    return domain->GetScore(position);
}

void Backprop(struct Node* n, enum CAS_Player winner)
{
    do
    {
        n->playouts++;
        if (n->player == winner)
            n->wins++;
        else if (winner == NONE)
            n->draws++;

        n = n->parent;

    } while (n != NULL);
}

/* Parse the tree to find the best action. */
void CAS_GetBestAction(void* state, struct CAS_ActionStats* stats)
{
    struct CAS_State* cas;
    struct Node* bestNode, *currentNode, *root;
    int mostPlayouts = 0;
    size_t i;

    cas = (struct CAS_State*)state;
    root = cas->root;
    for (i = 0; i < root->children->numNodes; i++)
    {
        currentNode = &root->children->nodes[i];
        if (currentNode->playouts > mostPlayouts)
        {
            mostPlayouts = currentNode->playouts;
            bestNode = currentNode;
        }
    }

    stats->action = bestNode->action;
    stats->winRate = (bestNode->wins + 0.5*bestNode->draws)/mostPlayouts;
    stats->playouts = mostPlayouts;
}

enum CAS_SearchResult CAS_Search(void* state,   
                                 CAS_DomainState initialPosition,
                                 enum CAS_Player player,
                                 int secs)
{
    struct CAS_State* cas;
    struct CAS_Domain* domain;
    struct CAS_ActionList* actionList;
    CAS_DomainState pos;
    enum CAS_Player winner;
    struct Node* n;
    time_t startTime;
    uint64_t prngState[2];

    cas = (struct CAS_State*)state;
    if (cas == NULL)
        return INSUFFICIENT_MEMORY;

    /* Initialise the root node. */
    cas->root = MakeRoot(cas->mem, player);
    if (cas->root == NULL)
        return INSUFFICIENT_MEMORY;

    /* Seed the PRNG. */
    prngState[0] = 0xdeadbeefcafebabe;
    prngState[1] = 0x8badf00dbaada555;

    /* Perform the MCTS procedure while resources remain. */
    domain = cas->domain;
    actionList = GetActionList(cas);
    startTime = time(NULL);
    pos = GetMemory(cas->mem, domain->domainStateSize);
    if (pos == NULL)
        return INSUFFICIENT_MEMORY;

    do 
    {
        domain->CopyState(initialPosition, pos);

        n = Select(domain, cas->root, pos);
        n = Expand(cas, domain, n, pos, actionList);
        if (n == NULL) break;
        winner = Simulate(domain, pos, actionList, prngState);
        Backprop(n, winner);

    } while (difftime(time(NULL), startTime) < secs);

    /* Now examine the tree to find the best move. */
    return SUCCESS;
}
