#ifndef __CASINO_INCLUDED_H__
#define __CASINO_INCLUDED_H__

/*
 * This header file defines the API for the Casino library.
 */

#include "stdint.h"
#include "stdlib.h"

typedef uint64_t CAS_Action;
typedef void* CAS_DomainState;

static const CAS_Action BAD_ACTION = ~(CAS_Action)0;

enum CAS_Player
{
    NONE, /* This constant is used to indicate a draw. */
    P1,
    P2 
};

enum CAS_SearchResult
{
    SUCCESS,
    INSUFFICIENT_MEMORY
};

struct CAS_ActionList
{
    CAS_Action* actions;
    size_t numActions;
};

struct CAS_ActionStats
{
    CAS_Action action;
    double winRate;
    int playouts;
};

/*
 * This struct contains the functions which define the problem domain.
 * All of these functions must be defined by the application.
 */
struct CAS_Domain
{
    /* The maximum number of actions that can be taken in any domain state. */
    int maxActionsPerTurn;

    /* The size of the domain state. */
    size_t domainStateSize;

    /* Create a copy of an existing domain state. */
    void (*CopyState)(CAS_DomainState, char*);

    /* Get a list of all available actions in this domain state. */
    void (*GetStateActions)(CAS_DomainState, struct CAS_ActionList*);

    /* Apply the specified action to the domain state. */
    void (*DoAction)(CAS_DomainState, CAS_Action);

    /*
     * Determine the result in the specified domain state.
     * This will only be called if `GetStateActions` did not return any actions,
     * which is interpreted as the domain reaching a terminal state.
     */
    enum CAS_Player (*GetScore)(CAS_DomainState);
};

/*
 * Initialise the Casino internals for the specified domain.
 * The application must provide *all* of the memory required by Casino - the 
 * library does not allocate any memory.
 * If Casino runs out of memory while searching then it will return the best
 * move so far.
 */
void* CAS_Init(struct CAS_Domain* domain, size_t bufSize, char* buf);

/* Search the current domain state. */
enum CAS_SearchResult CAS_Search(void* cas,
                                 CAS_DomainState initialPosition,
                                 enum CAS_Player player,
                                 int secs);

/* Get statistics for the best action in the most recent search. */
void CAS_GetBestAction(void* cas, struct CAS_ActionStats* stats);

/* Methods to creating and interacting with action lists. */
void CAS_AddAction(struct CAS_ActionList* list, CAS_Action action);

#endif /* __CASINO_INCLUDED_H__ */
