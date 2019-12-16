#include "../include/casino.h"
#include "assert.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

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

bool IsGameOver(struct TTTState* ttt)
{
    bool boardFull = true;
    int i;

    if (CheckForWinner(ttt) != NONE)
        return true;

    for (i = 0; i < 9 && boardFull; i++)
    {
        boardFull = ttt->pieces[i] != NONE;
    }

    return boardFull;
}

char GetPieceChar(enum CAS_Player player)
{
    return player == P1 ? 'O' : player == P2 ? 'X' : ' ';
}

void PrintState(struct TTTState* state)
{
    printf("3  %c | %c | %c\n"
           "   ---------\n"
           "2  %c | %c | %c\n"
           "   ---------\n"
           "1  %c | %c | %c\n"
           "   A   B   C \n\n",
           GetPieceChar(state->pieces[0]),
           GetPieceChar(state->pieces[1]),
           GetPieceChar(state->pieces[2]),
           GetPieceChar(state->pieces[3]),
           GetPieceChar(state->pieces[4]),
           GetPieceChar(state->pieces[5]),
           GetPieceChar(state->pieces[6]),
           GetPieceChar(state->pieces[7]),
           GetPieceChar(state->pieces[8]));
}

CAS_Action GetUserAction()
{
    char buf[3];
    int r, c;

    if (fgets(buf, 3, stdin) == NULL)
    {
        printf("Error reading stdin.\n");
        assert(false);
    }

    while ((c = getchar()) != '\n' && c != EOF) { }

    c = buf[0]-'A';
    r = 2+'1'-buf[1];

    return 3*r+c;
}

void PlayGame(void* casState, struct CAS_SearchConfig* config)
{
    struct TTTState* tttState;
    struct CAS_ActionStats* stats;
    enum CAS_SearchResult res;
    CAS_Action userAction;

    tttState = MakeState();
    stats = (struct CAS_ActionStats*)malloc(sizeof(struct CAS_ActionStats));

    while (!IsGameOver(tttState))
    {
        /* Do the computer move. */
        printf("Starting search.\n");
        res = CAS_Search(casState, config, tttState, P1, 500);
        if (res != SUCCESS)
        {
            printf("Search failed: %d\n", res);
            break;
        }

        printf("Search complete.\n");

        CAS_GetBestAction(casState, stats);
        printf("Action stats:\n"
               "Best move: %ld\n"
               "Win rate: %f\n"
               "Playouts: %d\n",
               stats->action, stats->winRate, stats->playouts);

        DoAction(tttState, stats->action);

        PrintState(tttState);

        if (IsGameOver(tttState))
            break;

        /* Do the human move. */
        userAction = GetUserAction();
        DoAction(tttState, userAction);

        PrintState(tttState);
    }

    free(stats);
    free(tttState);
}

int main()
{
    const int MaxActionsPerTurn = 9;
    const size_t MaxBytes = 50*1024*1024;
    struct CAS_Domain domain;
    struct CAS_SearchConfig config;
    void* casState;
    char* buf;

    /* Initialise the problem domain. */
    domain.maxActionsPerTurn = MaxActionsPerTurn;
    domain.domainStateSize = sizeof(struct TTTState);
    domain.CopyState = &CopyState;
    domain.GetStateActions = &GetStateActions;
    domain.DoAction = &DoAction;
    domain.GetScore = &GetScore;

    /* Initialise the search config. */
    /* In this case we just use the defaults. */
    config.SelectionPolicy = &CAS_DefaultSelectionPolicy;
    config.PlayoutPolicy = &CAS_DefaultPlayoutPolicy;

    /* Initialise Casino. */
    buf = (char*)malloc(MaxBytes);

    casState = CAS_Init(&domain, MaxBytes, buf);
    if (casState == NULL)
    {
        printf("Oh dear!\n");
        return -1;
    }

    PlayGame(casState, &config);

    free(buf);

    return 0;
}
