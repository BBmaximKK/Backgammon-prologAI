#include "graphics/state.h"
#include <stdbool.h>
#include <string.h>

void InitBoard(Board *b){

    memset(b, 0, sizeof(Board));

    // ── SETUP INIZIALE ────────────────────────────────────────────
    // points[i] > 0 = pedine bianche, < 0 = pedine nere
 
    // Bianche
    b->points[5]  =  5;
    b->points[7]  =  3;
    b->points[12] =  5;
    b->points[23] =  2;
 
    // Nere
    b->points[0]  = -2;
    b->points[11] = -5;
    b->points[16] = -3;
    b->points[18] = -5;
 
    // Bar e bear-off
    b->barWhite   = b->barBlack   = 0;
    b->bearoffWhite = b->bearoffBlack = 0;
 
    // PIP iniziale
    b->pipWhite = 167;
    b->pipBlack = 167;
 
    // Punteggio
    b->scoreWhite = 0;
    b->scoreBlack = 0;
    b->firstMover = 0;
 
    // Highlight mosse
    b->lastMovedCount = 0;
    for(int i = 0; i < 4; i++){
        b->lastMovedFrom[i] = -1;
        b->lastMovedTo[i]   = -1;
    }
 
    b->gameStarted = false;
}