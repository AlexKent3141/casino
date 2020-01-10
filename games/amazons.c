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

#define BOARD_WIDTH 10
#define PADDED_BOARD_WIDTH (BOARD_WIDTH + 1)
#define PADDED_BOARD_SIZE (PADDED_BOARD_WIDTH * PADDED_BOARD_WIDTH)
#define NUM_PIECES 4

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
    for (r = BOARD_WIDTH; r > 0; r--)
    {
        printf(r < 10 ? "%d  |" : "%d |", r);
        for (c = 0; c < BOARD_WIDTH; c++) PrintLoc(state, r-1, c);
        printf("\n   ---------------------\n");
    }

    printf("    A B C D E F G H I J\n");
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
    st->p1PieceLocs[0] = 4; st->p1PieceLocs[1] = 7;
    st->p1PieceLocs[2] = 34; st->p1PieceLocs[3] = 43;
    st->p2PieceLocs[0] = 67; st->p2PieceLocs[1] = 76;
    st->p2PieceLocs[2] = 103; st->p2PieceLocs[3] = 106;
    
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
    memcpy(copy->p1PieceLocs, orig->p1PieceLocs, NUM_PIECES*sizeof(int));
    memcpy(copy->p2PieceLocs, orig->p1PieceLocs, NUM_PIECES*sizeof(int));
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

void PrintAction(CAS_Action action)
{
    /*
    int start = GetStart(action), end = GetEnd(action);
    int r1 = start / 8, c1 = start % 8, r2 = end / 8, c2 = end % 8;
    printf("%c%d%c%d\n", c1 + 'A', r1 + 1, c2 + 'A', r2 + 1);
    */
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

    return 0;/*MakeAction(8*r1+c1, 8*r2+c2);*/
}

void PlayGame(void* casState, struct CAS_SearchConfig* config)
{
    struct AmazonsState* amazonsState;
    struct CAS_ActionStats* stats;
    enum CAS_SearchResult res;
    CAS_Action userAction;

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
        printf("Action stats:\n");
        PrintAction(stats->action);
        printf("Win rate: %f\n"
               "Playouts: %d\n",
               stats->winRate, stats->playouts);

        DoAction(amazonsState, stats->action);

        PrintState(amazonsState);

        if (GetScore(amazonsState) != NONE)
            break;

        /* Do the human move. */
        userAction = GetUserAction();
        DoAction(amazonsState, userAction);

        PrintState(amazonsState);
    }

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
