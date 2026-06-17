#include "game.h"
#include "engine/moves.h"
#include "engine/ai.h"
#include "raylib.h"
#include <stdio.h>
 
// ─────────────────────────────────────────────────────────────────
// CheckWin — ritorna 1 se il bianco ha vinto, -1 se il nero, 0 altrimenti
// ─────────────────────────────────────────────────────────────────
static int CheckWin(Board *b){
    if(b->bearoffWhite >= 15) return  1;
    if(b->bearoffBlack >= 15) return -1;
    return 0;
}
 
// ─────────────────────────────────────────────────────────────────
// UpdateGame — macchina a stati per turno AI vs AI
//
//   GAME_ROLL_DICE   → lancia i dadi, gestisce doppio
//   GAME_AI_THINK    → l'AI calcola e applica la mossa
//   GAME_SWITCH_TURN → cambia giocatore, torna a ROLL_DICE
//   GAME_OVER        → partita finita, no-op
// ─────────────────────────────────────────────────────────────────
void UpdateGame(Game *g){
    if(!g->running || g->paused) return;

    g->turnTimer += GetFrameTime();
    if(g->turnTimer < 0.8f) return;
    g->turnTimer = 0.0f;

    switch(g->phase)
    {
    case GAME_ROLL_DICE:
        RollDice(&g->dice);
        if(g->dice.isDouble)
            printf("=== DOPPIO! Turno %s | dado %d x4 ===\n",
                   g->currentPlayer == PLAYER_WHITE ? "BIANCO" : "NERO",
                   g->dice.d1);
        else
            printf("--- Turno %s | dadi %d %d ---\n",
                   g->currentPlayer == PLAYER_WHITE ? "BIANCO" : "NERO",
                   g->dice.d1, g->dice.d2);
        g->phase = GAME_AI_THINK;
        break;

    case GAME_AI_THINK:
        AI_PlayTurn(&g->board, g->dice, g->currentPlayer);

        {
            int w = CheckWin(&g->board);
            if(w != 0){
                printf("*** PARTITA FINITA: vince il %s! ***\n",
                       w == 1 ? "BIANCO" : "NERO");

                // aggiorna punteggio
                int sw = g->board.scoreWhite + (w ==  1 ? 1 : 0);
                int sb = g->board.scoreBlack + (w == -1 ? 1 : 0);

                // reset tavola immediato
                InitBoard(&g->board);
                g->board.scoreWhite = sw;
                g->board.scoreBlack = sb;

                // ferma il gioco e torna allo stato iniziale
                g->running       = false;
                g->paused        = false;
                g->turn          = 0;
                g->turnTimer     = 0.0f;
                g->phase         = GAME_ROLL_DICE;
                g->currentPlayer = PLAYER_WHITE;
                break;
            }
        }
        g->phase = GAME_SWITCH_TURN;
        break;

    case GAME_SWITCH_TURN:
        g->currentPlayer = (g->currentPlayer == PLAYER_WHITE
                            ? PLAYER_BLACK
                            : PLAYER_WHITE);
        g->phase = GAME_ROLL_DICE;
        break;

    case GAME_OVER:
        break;

    default:
        g->phase = GAME_ROLL_DICE;
        break;
    }
}