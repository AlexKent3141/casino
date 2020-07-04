#ifndef __CASINO_MCTS_INCLUDED_H__
#define __CASINO_MCTS_INCLUDED_H__

#include "../include/casino.h"
#include "casino_state.h"
#include "node.h"

/* Keep selecting child nodes until a leaf node (or a terminal node) is reached. */
struct CAS_Node* Select(
    void* cas,
    struct CAS_SearchConfig* config,
    struct CAS_Domain* domain,
    struct CAS_Node* n,
    CAS_DomainState position);

/* Add nodes for each of the actions available from n and pick the first. */
struct CAS_Node* Expand(
    struct CAS_State* cas,
    struct CAS_SearchConfig* config,
    struct CAS_Domain* domain,
    struct CAS_Node* n,
    CAS_DomainState position,
    struct CAS_ActionList* actionList);

/* Play out a random game from the point and score the terminal state. */
enum CAS_Player Simulate(
    struct PRNGState* st,
    struct CAS_SearchConfig* config,
    struct CAS_Domain* domain,
    CAS_DomainState position,
    struct CAS_ActionList* actionList);

/* Back propagate the result of a playout up the tree. */
void Backprop(
    struct CAS_Node* n,
    enum CAS_Player winner);

/* The data which needs to be passed to each worker thread. */
struct WorkerData
{
    struct CAS_State* cas;
    struct CAS_SearchConfig* config;
    CAS_DomainState initialPosition;
    struct CAS_ActionList* actionList;
    CAS_DomainState workerPosition;
    PRNGState* prngState;
    pthread_mutex_t* treeLock;
};

/* This function is executed by a worker thread. */
void* SearchWorker(void*);

#endif /* __CASINO_MCTS_INCLUDED_H__ */
