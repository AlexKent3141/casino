#include "../include/casino.h"

#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"

/* Test Casino using 8x8 Breakthrough as an example. */

/*
 * An 8x8 board is very conducive to bitboard move generation.
 */

typedef uint64_t bb;

/*
 * Initialise the bitboards that we need.
 */

bb rows[8], cols[8];
bb squares[64];

void InitBitboards()
{
    bb row = 0x00000000000000FF;
    bb col = 0x0101010101010101;
    bb square = 0x1;
    int i;
    for (i = 0; i < 8; i++)
    {
        rows[i] = row;
        cols[i] = col;
        row <<= 8;
        col <<= 1;
    }

    for (i = 0; i < 64; i++)
    {
        squares[i] = square;
        square <<= 1;
    }
}

struct BreakState
{
    enum CAS_Player player;
    bb p1Pieces;
    bb p2Pieces;
};

/* Helper functions. */
struct BreakState* MakeState()
{
    struct BreakState* st = (struct BreakState*)malloc(sizeof(struct BreakState));
    st->player = CAS_P1;
    st->p1Pieces = rows[0] | rows[1];
    st->p2Pieces = rows[6] | rows[7];
    return st;
}

enum CAS_Player CheckForWinner(struct BreakState* st)
{
    if (st->p1Pieces & rows[7])
        return CAS_P1;
    else if (st->p2Pieces & rows[0])
        return CAS_P2;
    else
        return CAS_NONE;
}

CAS_Action MakeAction(int start, int end)
{
    return start + (end << 8);
}

int GetStart(CAS_Action action)
{
    return action & 0xFF;
}

int GetEnd(CAS_Action action)
{
    return action >> 8;
}

int PopLSB(bb* bits)
{
    int bit = __builtin_ffsll(*bits)-1;
    *bits &= *bits - 1;
    return bit;
}

/* Methods for the custom MCTS policies used in this application. */
CAS_Action GetWinningMove(CAS_DomainState st)
{
    struct BreakState* board = (struct BreakState*)st;
    CAS_Action winningMove = CAS_BAD_ACTION;
    bb friendly, winners;
    int loc;

    friendly = board->player == CAS_P1 ? board->p1Pieces : board->p2Pieces;
    winners = board->player == CAS_P1 ? friendly & rows[6] : friendly & rows[1];

    if (winners)
    {
        /* Choose a diagonal winning move (can't be blocked). */
        if (winners & ~cols[0])
        {
            winners &= ~cols[0];
            loc = PopLSB(&winners);
            winningMove = MakeAction(loc, board->player == CAS_P1 ? loc + 7 : loc - 9);
        }
        else
        {
            winners &= cols[0];
            loc = PopLSB(&winners);
            winningMove = MakeAction(loc, board->player == CAS_P1 ? loc + 9 : loc - 7);
        }
    }

    return winningMove;
}

void GetCaptures(CAS_DomainState st, struct CAS_ActionList* actions)
{
    struct BreakState* board = (struct BreakState*)st;
    bb friendly, enemy, leftDiag, rightDiag, temp;
    int start, end;

    friendly = board->player == CAS_P1 ? board->p1Pieces : board->p2Pieces;
    enemy = board->player == CAS_P1 ? board->p2Pieces : board->p1Pieces;

    temp = enemy & ~cols[7];
    leftDiag = board->player == CAS_P1 ? temp >> 7 : temp << 9;
    leftDiag &= friendly;

    temp = enemy & ~cols[0];
    rightDiag = board->player == CAS_P1 ? temp >> 9 : temp << 7;
    rightDiag &= friendly;

    while (leftDiag)
    {
        start = PopLSB(&leftDiag);
        end = board->player == CAS_P1 ? start + 7 : start - 9;
        CAS_AddAction(actions, MakeAction(start, end));
    }
    while (rightDiag)
    {
        start = PopLSB(&rightDiag);
        end = board->player == CAS_P1 ? start + 9 : start - 7;
        CAS_AddAction(actions, MakeAction(start, end));
    }
}

CAS_Action GetRandomCapture(void* casState,
                            CAS_DomainState st,
                            struct CAS_ActionList* list)
{
    CAS_Action cap = CAS_BAD_ACTION;
    GetCaptures(st, list);
    if (list->numActions > 0)
    {
        cap = list->actions[CAS_Random(casState, list->numActions)];
    }

    return cap;
}

/* This node scoring function introduces bias towards capturing moves. */
double BiasedNodeScore(CAS_DomainState position, struct CAS_Node* n)
{
    const double CaptureWeight = 1000;
    const double ExplorationConstant = sqrt(2);

    struct BreakState* board = (struct BreakState*)position;
    bb enemy = board->player == CAS_P1 ? board->p2Pieces : board->p1Pieces;
    double score = 0;

    if (enemy & squares[GetEnd(n->action)])
    {
        /* Calculate the bias term. */
        /* This is formulated so that the bias will be gradually discarded as
           move playouts are performed. In the literature this is called
           progressive bias. */
        score += CaptureWeight / n->playouts;
    }

    /* This is the standard UCB formula. */
    score += CAS_WinRate(n) + CAS_UCBExploration(n, ExplorationConstant);

    return score;
}

struct CAS_Node* BiasedSelectionPolicy(void* casState,
                                       CAS_DomainState position,
                                       struct CAS_Node* parent)
{
    (void)casState;
    return CAS_SelectByScore(parent, position, &BiasedNodeScore);
}

CAS_Action BiasedPlayoutPolicy(void* casState,
                                 struct CAS_Domain* domainState,
                                 CAS_DomainState position,
                                 struct CAS_ActionList* list)
{
    /* Make sure that the game isn't over already. */
    CAS_Action action = CAS_BAD_ACTION;
    if (CheckForWinner(position) != CAS_NONE)
        return action;

    /* If a winning move is available then play it. */
    action = GetWinningMove(position);
    if (action != CAS_BAD_ACTION)
        return action;

    /* If there are captures available then play one with high probability. */
    action = GetRandomCapture(casState, position, list);
    if (action != CAS_BAD_ACTION)
    {
        if (CAS_Random(casState, 3) > 0)
        {
            return action;
        }
    }

    /* Default policy. */
    return CAS_DefaultPlayoutPolicy(casState, domainState, position, list);
}

/* From here on these are the functions required by Casino. */
void CopyState(CAS_DomainState st, char* buf)
{
    struct BreakState* copy, *orig;
    orig = (struct BreakState*)st;
    copy = (struct BreakState*)buf;
    copy->player = orig->player;
    copy->p1Pieces = orig->p1Pieces;
    copy->p2Pieces = orig->p2Pieces;
}

void GetStateActions(CAS_DomainState st, struct CAS_ActionList* actions)
{
    struct BreakState* board = (struct BreakState*)st;
    bb forward, leftDiag, rightDiag, friendly, enemy, all, empty, temp;
    int start, end;

    /* Check whether the game is already over. */
    if (CheckForWinner(board) != CAS_NONE)
        return;

    friendly = board->player == CAS_P1 ? board->p1Pieces : board->p2Pieces;
    enemy = board->player == CAS_P1 ? board->p2Pieces : board->p1Pieces;
    all = friendly | enemy;
    empty = ~all;

    /* Generate the available moves for the current player. */
    /* Pieces can only go forward to empty spaces, but can either move
       or capture diagonally forward. */
    forward = board->player == CAS_P1
        ? (empty >> 8) & friendly
        : (empty << 8) & friendly;

    temp = ~friendly & ~cols[7];
    leftDiag = board->player == CAS_P1 ? temp >> 7 : temp << 9;
    leftDiag &= friendly;

    temp = ~friendly & ~cols[0];
    rightDiag = board->player == CAS_P1 ? temp >> 9 : temp << 7;
    rightDiag &= friendly;

    /* We have the pieces which move in each direction, now produce moves. */
    while (forward)
    {
        start = PopLSB(&forward);
        end = board->player == CAS_P1 ? start + 8 : start - 8;
        CAS_AddAction(actions, MakeAction(start, end));
    }
    while (leftDiag)
    {
        start = PopLSB(&leftDiag);
        end = board->player == CAS_P1 ? start + 7 : start - 9;
        CAS_AddAction(actions, MakeAction(start, end));
    }
    while (rightDiag)
    {
        start = PopLSB(&rightDiag);
        end = board->player == CAS_P1 ? start + 9 : start - 7;
        CAS_AddAction(actions, MakeAction(start, end));
    }
}

void DoAction(CAS_DomainState st, CAS_Action action)
{
    struct BreakState* board = (struct BreakState*)st;
    int start = GetStart(action), end = GetEnd(action);
    
    if (board->player == CAS_P1)
    {
        board->p1Pieces ^= (squares[start] | squares[end]);
        board->p2Pieces &= ~squares[end];
    }
    else
    {
        board->p2Pieces ^= (squares[start] | squares[end]);
        board->p1Pieces &= ~squares[end];
    }

    board->player = board->player == CAS_P1 ? CAS_P2 : CAS_P1;
}

enum CAS_Player GetScore(CAS_DomainState st)
{
    struct BreakState* board = (struct BreakState*)st;
    return CheckForWinner(board);
}

/* Methods required for CLI. */
void PrintLoc(struct BreakState* state, int loc)
{
    if (state->p1Pieces & squares[loc]) printf("O|");
    else if (state->p2Pieces & squares[loc]) printf("X|");
    else printf(".|");
}

void PrintState(struct BreakState* state)
{
    int r, c;
    printf("  -----------------\n");
    for (r = 8; r > 0; r--)
    {
        printf("%d |", r);
        for (c = 0; c < 8; c++) PrintLoc(state, 8*(r-1)+c);
        printf("\n  -----------------\n");
    }

    printf("   A B C D E F G H\n");
}

void PrintAction(CAS_Action action)
{
    int start = GetStart(action), end = GetEnd(action);
    int r1 = start / 8, c1 = start % 8, r2 = end / 8, c2 = end % 8;
    printf("%c%d%c%d\n", c1 + 'A', r1 + 1, c2 + 'A', r2 + 1);
}

CAS_Action GetUserAction()
{
    char buf[5];
    int c, r1, c1, r2, c2;

    if (fgets(buf, 5, stdin) == NULL)
    {
        printf("Error reading stdin.\n");
    }

    while ((c = getchar()) != '\n' && c != EOF) { }

    c1 = buf[0]-'A';
    r1 = buf[1]-'1';
    c2 = buf[2]-'A';
    r2 = buf[3]-'1';

    return MakeAction(8*r1+c1, 8*r2+c2);
}

void PlayGame(void* casState, struct CAS_SearchConfig* config)
{
    struct BreakState* breakState;
    struct CAS_ActionStats* stats;
    enum CAS_SearchResult res;
    CAS_Action userAction;

    breakState = MakeState();
    stats = (struct CAS_ActionStats*)malloc(sizeof(struct CAS_ActionStats));

    while (CheckForWinner(breakState) == CAS_NONE)
    {
        /* Do the computer move. */
        printf("Starting search.\n");
        res = CAS_Search(casState, config, breakState, CAS_P1, 5000);
        if (res != CAS_SUCCESS)
        {
            printf("Search failed: %d\n", res);
            break;
        }

        printf("Search complete.\n");

        CAS_GetBestAction(casState, stats);
        printf("Action stats:\n");
        PrintAction(stats->action);
        printf("Win rate: %f\n"
               "Playouts: %d\n",
               stats->winRate, stats->playouts);

        DoAction(breakState, stats->action);

        PrintState(breakState);

        if (CheckForWinner(breakState) != CAS_NONE)
            break;

        /* Do the human move. */
        userAction = GetUserAction();
        DoAction(breakState, userAction);

        PrintState(breakState);
    }

    free(stats);
    free(breakState);
}

int main()
{
    const int MaxActionsPerTurn = 48;
    const size_t MaxBytes = 500*1024*1024;
    struct CAS_Domain domain;
    struct CAS_SearchConfig config;
    void* casState;
    char* buf;

    /* Initialise the problem domain. */
    domain.maxActionsPerTurn = MaxActionsPerTurn;
    domain.actionStages = 1;
    domain.domainStateSize = sizeof(struct BreakState);
    domain.CopyState = &CopyState;
    domain.GetStateActions = &GetStateActions;
    domain.DoAction = &DoAction;
    domain.GetScore = &GetScore;

    /* Initialise the search config. */
    config.numThreads = 4;
    config.SelectionPolicy = &BiasedSelectionPolicy;
    config.PlayoutPolicy = &BiasedPlayoutPolicy;

    /* Initialise Casino. */
    buf = (char*)malloc(MaxBytes);
    casState = CAS_Init(&domain, MaxBytes, buf);
    if (casState == NULL)
    {
        printf("Oh dear!\n");
        return -1;
    }

    InitBitboards();
    PlayGame(casState, &config);

    free(buf);

    return 0;
}
