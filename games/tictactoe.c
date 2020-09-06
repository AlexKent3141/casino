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
    st->player = CAS_P1;
    memset(st->pieces, CAS_NONE, 9*sizeof(enum CAS_Player));
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
    if (CheckPlayerIsWinner(st, CAS_P1))
        return CAS_P1;
    else if (CheckPlayerIsWinner(st, CAS_P2))
        return CAS_P2;
    else
        return CAS_NONE;
}

void CopyState(CAS_DomainState source, CAS_DomainState target)
{
    struct TTTState* copy, *orig;
    orig = (struct TTTState*)source;
    copy = (struct TTTState*)target;
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
    if (CheckForWinner(ttt) != CAS_NONE)
        return;

    /* Find where the current player can play. */
    for (i = 0; i < 9; i++)
    {
        if (ttt->pieces[i] == CAS_NONE)
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
    ttt->player = ttt->player == CAS_P1 ? CAS_P2 : CAS_P1;
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

    if (CheckForWinner(ttt) != CAS_NONE)
        return true;

    for (i = 0; i < 9 && boardFull; i++)
    {
        boardFull = ttt->pieces[i] != CAS_NONE;
    }

    return boardFull;
}

char GetPieceChar(enum CAS_Player player)
{
    return player == CAS_P1 ? 'O' : player == CAS_P2 ? 'X' : ' ';
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
    struct TTTState** workerStates;
    struct CAS_ActionStats* stats;
    enum CAS_SearchResult res;
    CAS_Action userAction;
    size_t i;

    tttState = MakeState();

    workerStates = (struct TTTState**)malloc(
        config->numThreads*sizeof(struct TTTState*));

    for (i = 0; i < config->numThreads; i++)
        workerStates[i] = MakeState();

    stats = (struct CAS_ActionStats*)malloc(sizeof(struct CAS_ActionStats));

    while (!IsGameOver(tttState))
    {
        /* Do the computer move. */
        printf("Starting search.\n");

        res = CAS_Search(casState, config, tttState, (void**)workerStates, CAS_P1, 500);
        if (res != CAS_SUCCESS)
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

    for (i = 0; i < config->numThreads; i++)
        free(workerStates[i]);

    free(workerStates);
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
    domain.actionStages = 1;
    domain.CopyState = &CopyState;
    domain.GetStateActions = &GetStateActions;
    domain.DoAction = &DoAction;
    domain.GetScore = &GetScore;

    /* Initialise the search config. */
    /* In this case we just use the defaults. */
    config.numThreads = 4;
    config.SelectionPolicy = &CAS_DefaultSelectionPolicy;
    config.PlayoutPolicy = &CAS_DefaultPlayoutPolicy;
    config.ExpansionPolicy = &CAS_DefaultExpansionPolicy;
    config.StopPlayout = &CAS_DefaultStopPlayoutCriterion;

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
