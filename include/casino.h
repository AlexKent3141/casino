#ifndef __CASINO_INCLUDED_H__
#define __CASINO_INCLUDED_H__

/*
 * This header file defines the API for the Casino library.
 */

#define MAJOR_VERSION 1
#define MINOR_VERSION 0

#include "stdbool.h"
#include "stdint.h"
#include "stdlib.h"

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#elif __GNUC__
#define EXPORT __attribute__ ((visibility("default")))
#else
#error "Compiler not supported"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t CAS_Action;
typedef void* CAS_DomainState;

static const CAS_Action CAS_BAD_ACTION = ~(CAS_Action)0;

enum CAS_Player
{
    CAS_NONE, /* This constant is used to indicate a draw. */
    CAS_P1,
    CAS_P2
};

enum CAS_SearchResult
{
    CAS_SUCCESS,
    CAS_INSUFFICIENT_MEMORY,
    CAS_COULD_NOT_INITIALISE_MUTEX
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

struct CAS_NodeList
{
    struct CAS_Node* nodes;
    size_t numNodes;
};

struct CAS_Node
{
    struct CAS_Node* parent;       /* The parent of this node. */
    enum CAS_Player player;        /* The player to move. */
    CAS_Action action;             /* The previous action. */
    int stage;                     /* The move stage of this node. */
    bool expanded;                 /* Whether this node has been expanded. */
    struct CAS_NodeList* children; /* The child nodes of this node. */
    int wins;                      /* The number of wins during playouts. */
    int draws;                     /* The number of draws during playouts. */
    int playouts;                  /* The total number of playouts. */
};

/*
 * This struct contains the functions which define the problem domain.
 * All of these functions must be defined by the application.
 */
struct CAS_Domain
{
    /* The maximum number of actions that can be taken in any domain state. */
    int maxActionsPerTurn;

    /* The number of stages each action is broken down into.
     * This is useful when implementing a game like Amazons where there is an
     * extremely high branching factor and it makes sense to break actions down
     * into sub-actions (in the case of Amazons: piece selection, piece movement,
     * firing arrows).
     */
    int actionStages;

    /* Create a copy of an existing domain state. */
    void (*CopyState)(
        CAS_DomainState source,
        CAS_DomainState target);

    /* Get a list of all available actions in this domain state. */
    void (*GetStateActions)(
        CAS_DomainState,
        struct CAS_ActionList*);

    /* Apply the specified action to the domain state. */
    void (*DoAction)(
        CAS_DomainState,
        CAS_Action);

    /*
     * Determine the result in the specified domain state.
     * This will only be called if `GetStateActions` did not return any actions,
     * which is interpreted as the domain reaching a terminal state.
     */
    enum CAS_Player (*GetScore)(
        CAS_DomainState);
};

struct CAS_SearchConfig
{
    size_t numThreads; /* The number of threads to use when searching. */

    /*
     * The selection policy to use.
     * This is used to select which node to expand next in the tree.
     */
    struct CAS_Node* (*SelectionPolicy)(
        void* cas,
        CAS_DomainState position,
        struct CAS_Node* parent);

    /*
     * The playout policy to use.
     * This is used during the simulation stage.
     */
    CAS_Action (*PlayoutPolicy)(
        void* prngState,
        struct CAS_Domain* domainState,
        CAS_DomainState position,
        struct CAS_ActionList* list);

    /*
     * This is the criterion that is used to terminate a playout.
     * The default implementation just terminates when the end of the game is
     * reached i.e. previousAction is CAS_BAD_ACTION.
     */
    bool (*StopPlayout)(
        CAS_Action previousAction,
        int playoutDepth);

    /*
     * This method is called when a node is expanded and its children are
     * generated. The idea is to assign the child nodes priorities based on
     * how promising they look accordingly to domain specific heuristics.
     */
    void (*PrioritiseExpandedNodesPolicy)(
        CAS_DomainState domainState,
        struct CAS_NodeList* nodes);

    /*
     * This criterion is used to determine whether a child of this parent node
     * should be expanded.
     */
    bool (*ExpansionPolicy)(
        struct CAS_Node* parent);
};

/*
 * Initialise the Casino internals for the specified domain.
 * The application must provide *all* of the memory required by Casino - the 
 * library does not allocate any memory.
 * If Casino runs out of memory while searching then it will return the best
 * move so far.
 */
EXPORT void* CAS_Init(
    struct CAS_Domain* domain,
    size_t bufSize,
    char* buf);

/* Search the current domain state. */
EXPORT enum CAS_SearchResult CAS_Search(
    void* cas,
    struct CAS_SearchConfig* config,
    CAS_DomainState initialPosition,
    CAS_DomainState* workerPositions,
    enum CAS_Player player,
    int ms);

/* Get statistics for the best action in the most recent search. */
EXPORT void CAS_GetBestAction(
    void* cas,
    struct CAS_ActionStats* stats);

/* Get the PV for the most recent search.
 * The return value is the depth of the PV which will be less than or equal to
 * the buffer size. */
EXPORT int CAS_GetPV(
    void* cas,
    int len,
    CAS_Action* buf);

/* Add an action to an action list. */
EXPORT void CAS_AddAction(
    struct CAS_ActionList* list,
    CAS_Action action);

/* Built-in selection policy methods. */

EXPORT double CAS_WinRate(
    struct CAS_Node* node);

EXPORT double CAS_UCBExploration(
    struct CAS_Node* node,
    double explorationFactor);

/*
 * Select the node with the highest score according to the scoring function.
 */
EXPORT struct CAS_Node* CAS_SelectByScore(
    struct CAS_Node* parent,
    CAS_DomainState position,
    double (*SelectScore)(
        CAS_DomainState,
        struct CAS_Node*));

/*
 * This playout termination criterion is used when we want to play out the game
 * to completion and then score the game state.
 */
EXPORT bool CAS_DefaultStopPlayoutCriterion(
    CAS_Action previousAction,
    int playoutDepth);

/*
 * This policy for prioritising expanded nodes does not have any effect.
 */
EXPORT void CAS_DefaultPrioritiseExpandedNodesPolicy(
    CAS_DomainState previousPosition,
    struct CAS_NodeList* childNodes);

/*
 * The default expansion policy just checks whether all children of the parent
 * have at least one playout.
 */
EXPORT bool CAS_DefaultExpansionPolicy(
    struct CAS_Node* parent);

/*
 * The default selection policy for nodes during selection.
 * This applies the UCB formula with exploration constant c = sqrt(2).
 */
EXPORT struct CAS_Node* CAS_DefaultSelectionPolicy(
    void* cas,
    CAS_DomainState position,
    struct CAS_Node* node);

/* Built-in playout policy methods. */

/*
 *  The default playout policy for action selection during playouts.
 * This applies uniform selection over the available actions.
 */
EXPORT CAS_Action CAS_DefaultPlayoutPolicy(
    void* prngState,
    struct CAS_Domain* domainState,
    CAS_DomainState position,
    struct CAS_ActionList* list);

/*
 * Get a pseudo-random number on the interval [0, bound).
 * This uses an internal xorshift generator.
 */
EXPORT int CAS_Random(
    void* prngState,
    int bound);

#ifdef __cplusplus
}
#endif

#endif /* __CASINO_INCLUDED_H__ */
