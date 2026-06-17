#ifndef STATE_H
#define STATE_H

#include <stdbool.h>
#define FIELD 24

// indici speciali
#define IDX_BAR_WHITE   24
#define IDX_BAR_BLACK   25
#define IDX_BEAROFF_W   26
#define IDX_BEAROFF_B   27

#define TOP_MOVES_N     5

typedef enum{
    EMPTY = 0,
    _WHITE = 1,
    _BLACK = -1
}PieceColor;

// Una mossa con score per il pannello Top-5
typedef struct{
    int   from;     // -1 = non valida
    int   to;       // IDX_BEAROFF_W/B o IDX_BAR_* o 0..23
    float score;
}TopMove;

typedef struct{
    int points[FIELD];
    int barWhite;
    int barBlack;
    int bearoffWhite;
    int bearoffBlack;

    int  pipWhite;
    int  pipBlack;
    int  scoreWhite;
    int  scoreBlack;
    int  firstMover;

    int lastMovedFrom[4];
    int lastMovedTo[4];
    int lastMovedCount;

    // top 5 mosse dell'ultimo turno (popolate da Prolog)
    TopMove topMoves[TOP_MOVES_N];
    int     topMovesCount;   // quante sono valide (0..5)

    bool gameStarted;
}Board;

void InitBoard(Board *b);

#endif