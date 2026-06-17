#pragma once

#include "../graphics/state.h"
#include "../game.h"

typedef struct{
    int from;
    int to;
}Move;

// Tenta la mossa e se legale la applica. Ritorna 1 se ok, 0 se illegale
int MoveChecker(Board *b, int from, int to, PlayerColor player);
// Controlla se un giocatore ha tutte le sue pedine nel proprio home board
int CanBearOff(Board *b, PlayerColor player);
// Wrapper logico
void ApplyMove(Board *b, Move *m, PlayerColor player);