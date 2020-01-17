#include "../include/casino.h"

#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

/* Test Casino using the Game of Amazons an example. */

/* Amazons has a 10x10 board so cannot be easily represented using bitboards.
   Instead use a (padded) letterbox style board representation.
   Padding is applied to the left and top edges of the board, so an 11x11 array
   is required. */

/* The branching factor for Amazons is extremely high. To deal with this I use
   move grouping to break each move down into 3 stages:
   - Piece selection: select which piece will move next.
   - Piece move: move the selected piece.
   - Arrow shot: shoot the arrow to an empty space. */

#ifdef MINI
#define BOARD_WIDTH 5
#else
#define BOARD_WIDTH 10
#endif

#define PADDED_BOARD_WIDTH (BOARD_WIDTH + 1)
#define PADDED_BOARD_SIZE (PADDED_BOARD_WIDTH * PADDED_BOARD_WIDTH)
#define NUM_PIECES 4
#define NUM_STAGES 3

/* Define the 8 directions that a chess queen can move in. */
#define NUM_DIRECTIONS 8
const int directions[NUM_DIRECTIONS] = { PADDED_BOARD_WIDTH-1,
                                         PADDED_BOARD_WIDTH,
                                         PADDED_BOARD_WIDTH+1,
                                         -1, 1,
                                         -PADDED_BOARD_WIDTH-1,
                                         -PADDED_BOARD_WIDTH,
                                         -PADDED_BOARD_WIDTH+1 };

enum TurnStage
{
    PIECE_SELECTION,
    PIECE_MOVE,
    ARROW_SHOT
};

struct AmazonsState
{
    enum CAS_Player player;

    int p1PieceLocs[NUM_PIECES];
    int p2PieceLocs[NUM_PIECES];

    /* All obstructions: amazons and arrows, plus -1's to represent off-board
       locations.*/
    int obstructions[PADDED_BOARD_SIZE];

    enum TurnStage stage;
    int selectedPieceIndex;
    int pieceTargetLoc;
};

/* Methods required for CLI. */
void PrintLoc(struct AmazonsState* st, int r, int c)
{
    int paddedLoc = PADDED_BOARD_WIDTH*r + c + 1;
    int i;

    /* First check whether there is a piece there. */
    for (i = 0; i < NUM_PIECES; i++)
    {
        if (st->p1PieceLocs[i] == paddedLoc)
        {
            printf("O|");
            return;
        }
        else if (st->p2PieceLocs[i] == paddedLoc)
        {
            printf("X|");
            return;
        }
    }

    /* Next check for an arrow. */
    if (st->obstructions[paddedLoc] == 1)
    {
        printf("*|");
        return;
    }

    /* Empty location. */
    printf(".|");
}

void PrintState(struct AmazonsState* state)
{
    int r, c;
    printf("   ---------------------\n");
    for (r = BOARD_WIDTH-1; r >= 0; r--)
    {
        printf("%d |", r);
        for (c = 0; c < BOARD_WIDTH; c++) PrintLoc(state, r, c);
        printf("\n   ---------------------\n");
    }

    printf("   A B C D E F G H I J\n");
}

/* Helper functions. */
struct AmazonsState* MakeState()
{
    struct AmazonsState* st =
        (struct AmazonsState*)malloc(sizeof(struct AmazonsState));
    int i;

    st->player = P1;
    st->stage = PIECE_SELECTION;

    /* Place the pieces in their starting positions (taking into account
       padding). */
#ifdef MINI
    st->p1PieceLocs[0] = 2; st->p1PieceLocs[1] = 4;
    st->p1PieceLocs[2] = 7; st->p1PieceLocs[3] = 11;
    st->p2PieceLocs[0] = 19; st->p2PieceLocs[1] = 23;
    st->p2PieceLocs[2] = 26; st->p2PieceLocs[3] = 28;
#else
    st->p1PieceLocs[0] = 4; st->p1PieceLocs[1] = 7;
    st->p1PieceLocs[2] = 34; st->p1PieceLocs[3] = 43;
    st->p2PieceLocs[0] = 67; st->p2PieceLocs[1] = 76;
    st->p2PieceLocs[2] = 103; st->p2PieceLocs[3] = 106;
#endif
    
    memset(st->obstructions, 0, PADDED_BOARD_SIZE*sizeof(int));
    for (i = 0; i < NUM_PIECES; i++)
    {
        st->obstructions[st->p1PieceLocs[i]] =  1;
        st->obstructions[st->p2PieceLocs[i]] =  1;
    }

    /* Setup the off-board boundary. */
    for (i = 0; i < PADDED_BOARD_WIDTH; i++)
    {
        st->obstructions[PADDED_BOARD_WIDTH*i] = 1;
        st->obstructions[PADDED_BOARD_SIZE-PADDED_BOARD_WIDTH+i] = 1;
    }

    return st;
}

/* From here on these are the functions required by Casino. */
void CopyState(CAS_DomainState st, char* buf)
{
    struct AmazonsState* orig = (struct AmazonsState*)st;
    struct AmazonsState* copy = (struct AmazonsState*)buf;

    copy->player = orig->player;
    copy->stage = orig->stage;
    copy->selectedPieceIndex = orig->selectedPieceIndex;
    copy->pieceTargetLoc = orig->pieceTargetLoc;
    memcpy(copy->p1PieceLocs, orig->p1PieceLocs, NUM_PIECES*sizeof(int));
    memcpy(copy->p2PieceLocs, orig->p2PieceLocs, NUM_PIECES*sizeof(int));
    memcpy(copy->obstructions, orig->obstructions, PADDED_BOARD_SIZE*sizeof(int));
}

bool Unobstructed(struct AmazonsState* board, int loc)
{
    return loc >= 0 && loc < PADDED_BOARD_SIZE && board->obstructions[loc] == 0;
}

void GetActionsInDirection(struct AmazonsState* board,
                           int startLoc,
                           int direction,
                           struct CAS_ActionList* actions)
{
    int loc = startLoc + direction;
    while (Unobstructed(board, loc))
    {
        CAS_AddAction(actions, loc);
        loc += direction;
    }
}

/* Check whether the specified piece has any moves available. */
bool CanMove(struct AmazonsState* board, int pieceLoc)
{
    int i, loc;
    for (i = 0; i < NUM_DIRECTIONS; i++)
    {
        loc = pieceLoc + directions[i];
        if (Unobstructed(board, loc))
        {
            return true;
        }
    }

    return false;
}

/* Get the possible pieces by index. */
void GetPieceSelectionActions(struct AmazonsState* board,
                              struct CAS_ActionList* actions)
{
    int i, loc;
    for (i = 0; i < NUM_PIECES; i++)
    {
        loc = board->player == P1 ? board->p1PieceLocs[i] : board->p2PieceLocs[i];
        if (CanMove(board, loc))
        {
            CAS_AddAction(actions, i);
        }
    }
}

void GetQueenMoveActions(struct AmazonsState* board,
                         int pieceLoc,
                         struct CAS_ActionList* actions)
{
    int i;
    for (i = 0; i < NUM_DIRECTIONS; i++)
    {
        GetActionsInDirection(board, pieceLoc, directions[i], actions);
    }
}

void GetPieceMoveActions(struct AmazonsState* board,
                         struct CAS_ActionList* actions)
{
    int pieceLoc = board->player == P1
        ? board->p1PieceLocs[board->selectedPieceIndex]
        : board->p2PieceLocs[board->selectedPieceIndex];

    GetQueenMoveActions(board, pieceLoc, actions);
}

void GetArrowShotActions(struct AmazonsState* board,
                         struct CAS_ActionList* actions)
{
    GetQueenMoveActions(board, board->pieceTargetLoc, actions);
}

void GetStateActions(CAS_DomainState st, struct CAS_ActionList* actions)
{
    struct AmazonsState* board = (struct AmazonsState*)st;

    switch (board->stage)
    {
        case PIECE_SELECTION:
            GetPieceSelectionActions(board, actions);
            break;
        case PIECE_MOVE:
            GetPieceMoveActions(board, actions);
            break;
        case ARROW_SHOT:
            GetArrowShotActions(board, actions);
            break;
    }
}

void DoAction(CAS_DomainState st, CAS_Action action)
{
    struct AmazonsState* board = (struct AmazonsState*)st;
    int loc;

    switch (board->stage)
    {
        case PIECE_SELECTION:
        {
            board->selectedPieceIndex = action;
            board->stage = PIECE_MOVE;
            break;
        }
        case PIECE_MOVE:
        {
            if (board->player == P1)
            {
                loc = board->p1PieceLocs[board->selectedPieceIndex];
                board->p1PieceLocs[board->selectedPieceIndex] = action;
            }
            else
            {
                loc = board->p2PieceLocs[board->selectedPieceIndex];
                board->p2PieceLocs[board->selectedPieceIndex] = action;
            }

            board->pieceTargetLoc = action;
            board->obstructions[loc] = 0;
            board->obstructions[action] = 1;
            board->stage = ARROW_SHOT;
            break;
        }
        case ARROW_SHOT:
        {
            board->obstructions[action] = 1;
            board->player = board->player == P1 ? P2 : P1;
            board->stage = PIECE_SELECTION;
            break;
        }
    }
}

/* A game of Amazons is a loss for the first player to run out of moves. */
enum CAS_Player GetScore(CAS_DomainState st)
{
    struct AmazonsState* board = (struct AmazonsState*)st;
    enum CAS_Player winner;
    int i;

    if (board->player == P1)
    {
        winner = P2;
        for (i = 0; i < NUM_PIECES && winner == P2; i++)
        {
            if (CanMove(board, board->p1PieceLocs[i]))
            {
                winner = NONE;
            }
        }
    }
    else
    {
        winner = P1;
        for (i = 0; i < NUM_PIECES && winner == P1; i++)
        {
            if (CanMove(board, board->p2PieceLocs[i]))
            {
                winner = NONE;
            }
        }
    }

    return winner;
}

void PrintMoveLoc(int loc)
{
    int r = loc / PADDED_BOARD_WIDTH;
    int c = loc % PADDED_BOARD_WIDTH;
    printf("%c%d", c - 1 + 'A', r);
}

/* Print a move composed of 3 actions. */
void PrintMove(struct AmazonsState* board, CAS_Action* actions)
{
    int pieceLoc = board->player == P1
        ? board->p1PieceLocs[actions[0]]
        : board->p2PieceLocs[actions[0]];

    PrintMoveLoc(pieceLoc);
    PrintMoveLoc(actions[1]);
    PrintMoveLoc(actions[2]);
    printf("\n");
}

int GetPieceIndex(struct AmazonsState* board, int loc)
{
    int i;
    for (i = 0; i < NUM_PIECES; i++)
    {
        if (board->p1PieceLocs[i] == loc || board->p2PieceLocs[i] == loc)
            return i;
    }

    return -1;
}

void GetUserMove(struct AmazonsState* board, CAS_Action* actions)
{
    char buf[7];
    int c, r1, c1, r2, c2, r3, c3;

    if (fgets(buf, 7, stdin) == NULL)
    {
        printf("Error reading stdin.\n");
    }

    while ((c = getchar()) != '\n' && c != EOF) { }

    c1 = buf[0]-'A'+1; r1 = buf[1]-'0';
    c2 = buf[2]-'A'+1; r2 = buf[3]-'0';
    c3 = buf[4]-'A'+1; r3 = buf[5]-'0';

    actions[0] = GetPieceIndex(board, PADDED_BOARD_WIDTH*r1 + c1);
    actions[1] = PADDED_BOARD_WIDTH*r2 + c2;
    actions[2] = PADDED_BOARD_WIDTH*r3 + c3;
    printf("%d %d %d\n", actions[0], actions[1], actions[2]);
}

void PlayGame(void* casState, struct CAS_SearchConfig* config)
{
    struct AmazonsState* amazonsState;
    struct CAS_ActionStats* stats;
    enum CAS_SearchResult res;
    CAS_Action* pv = (CAS_Action*)malloc(NUM_STAGES*sizeof(CAS_Action));
    int i;

    amazonsState = MakeState();

    stats = (struct CAS_ActionStats*)malloc(sizeof(struct CAS_ActionStats));

    while (GetScore(amazonsState) == NONE)
    {
        /* Do the computer move. */
        printf("Starting search.\n");
        res = CAS_Search(casState, config, amazonsState, P1, 500);
        if (res != SUCCESS)
        {
            printf("Search failed: %d\n", res);
            break;
        }

        printf("Search complete.\n");

        CAS_GetBestAction(casState, stats);
        printf("Action stats:\n"
               "Win rate: %f\n"
               "Playouts: %d\n",
               stats->winRate, stats->playouts);

        /* Must have search deep enough to define a 3 stage move. */
        i = CAS_GetPV(casState, NUM_STAGES, pv);
        if (i < NUM_STAGES)
        {
            printf("Could not search deep enough to define a move.\n");
            return;
        }

        PrintMove(amazonsState, pv);

        for (i = 0; i < NUM_STAGES; i++)
            DoAction(amazonsState, pv[i]);

        PrintState(amazonsState);

        if (GetScore(amazonsState) != NONE)
            break;

        /* Do the human move. */
        GetUserMove(amazonsState, pv);
        for (i = 0; i < NUM_STAGES; i++)
            DoAction(amazonsState, pv[i]);

        PrintState(amazonsState);
    }

    free(pv);
    free(stats);
    free(amazonsState);
}

int main()
{
    const int MaxActionsPerTurn = NUM_DIRECTIONS*(BOARD_WIDTH - 1);
    const size_t MaxBytes = 500*1024*1024;
    struct CAS_Domain domain;
    struct CAS_SearchConfig config;
    void* casState;
    char* buf;

    /* Initialise the problem domain. */
    domain.maxActionsPerTurn = MaxActionsPerTurn;
    domain.domainStateSize = sizeof(struct AmazonsState);
    domain.CopyState = &CopyState;
    domain.GetStateActions = &GetStateActions;
    domain.DoAction = &DoAction;
    domain.GetScore = &GetScore;

    /* Initialise the search config. */
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
