#include "../include/casino.h"
#include "node.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"
#include "string.h"
#include "assert.h"

/* Test Casino using tictactoe as an example. */
struct TTTState
{
    enum CAS_Player player;
    enum CAS_Player pieces[9];
};

struct TTTState* MakeState()
{
    struct TTTState* st = (struct TTTState*)malloc(sizeof(struct TTTState));
    st->player = P1;
    memset(st->pieces, NONE, 9*sizeof(enum CAS_Player));
    return st;
}

bool CheckPlayerIsWinner(struct TTTState* st, enum CAS_Player p)
{
    return (st->pieces[0] == p && st->pieces[1] == p && st->pieces[2] == p)
        || (st->pieces[3] == p && st->pieces[4] == p && st->pieces[5] == p)
        || (st->pieces[6] == p && st->pieces[7] == p && st->pieces[8] == p)
        || (st->pieces[0] == p && st->pieces[3] == p && st->pieces[6] == p)
        || (st->pieces[1] == p && st->pieces[4] == p && st->pieces[7] == p)
        || (st->pieces[2] == p && st->pieces[5] == p && st->pieces[8] == p)
        || (st->pieces[0] == p && st->pieces[4] == p && st->pieces[8] == p)
        || (st->pieces[2] == p && st->pieces[4] == p && st->pieces[6] == p);
}

enum CAS_Player CheckForWinner(struct TTTState* st)
{
    if (CheckPlayerIsWinner(st, P1))
        return P1;
    else if (CheckPlayerIsWinner(st, P2))
        return P2;
    else
        return NONE;
}

void CopyState(CAS_DomainState st, char* buf)
{
    struct TTTState* copy, *orig;
    orig = (struct TTTState*)st;
    copy = (struct TTTState*)buf;
    assert(copy != NULL);
    memcpy(copy->pieces, orig->pieces, 9*sizeof(enum CAS_Player));
    copy->player = orig->player;
}

void GetStateActions(CAS_DomainState st, struct CAS_ActionList* actions)
{
    struct TTTState* ttt;
    int i;

    ttt = (struct TTTState*)st;

    /* Check whether a player has won. */
    if (CheckForWinner(ttt) != NONE)
        return;

    /* Find where the current player can play. */
    for (i = 0; i < 9; i++)
    {
        if (ttt->pieces[i] == NONE)
        {
            CAS_AddAction(actions, i);
        }
    }
}

void DoAction(CAS_DomainState st, CAS_Action action)
{
    struct TTTState* ttt;
    ttt = (struct TTTState*)st;
    ttt->pieces[action] = ttt->player;
    ttt->player = ttt->player == P1 ? P2 : P1;
}

enum CAS_Player GetScore(CAS_DomainState st)
{
    struct TTTState* ttt;
    ttt = (struct TTTState*)st;
    return CheckForWinner(ttt);
}

int main()
{
    const int MaxActionsPerTurn = 9;
    const size_t MaxBytes = 1048576*10;
    struct TTTState* tttState;
    struct CAS_Domain domain;
    void* casState;
    char* buf;
    enum CAS_SearchResult res;
    struct CAS_ActionStats* stats;

    /* Initialise TTT state. */
    domain.maxActionsPerTurn = MaxActionsPerTurn;
    domain.domainStateSize = sizeof(struct TTTState);
    domain.CopyState = &CopyState;
    domain.GetStateActions = &GetStateActions;
    domain.DoAction = &DoAction;
    domain.GetScore = &GetScore;

    /* Initialise Casino. */
    buf = (char*)malloc(MaxBytes);

    casState = CAS_Init(&domain, MaxBytes, buf);
    if (casState == NULL)
    {
        printf("Oh dear!\n");
        return -1;
    }

    tttState = MakeState();
    res = CAS_Search(casState, tttState, P1, 10);
    if (res != SUCCESS)
    {
        printf("Search failed: %d\n", res);
        return -1;
    }

    printf("Search complete.\n");

    stats = (struct CAS_ActionStats*)malloc(sizeof(struct CAS_ActionStats));
    CAS_GetBestAction(casState, stats);
    printf("Action stats:\n"
           "Best move: %ld\n"
           "Win rate: %f\n"
           "Playouts: %d\n",
           stats->action, stats->winRate, stats->playouts);

    free(stats);
    free(tttState);
    free(buf);

    return 0;
}
