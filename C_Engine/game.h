#pragma once
 
#include "graphics/state.h"
#include "engine/dice.h"
#include <stdbool.h>
 
typedef enum{
    PLAYER_WHITE = 1,
    PLAYER_BLACK = -1
}PlayerColor;
 
typedef enum{
    GAME_ROLL_DICE = 0,
    GAME_AI_THINK,
    GAME_APPLY_MOVE,
    GAME_SWITCH_TURN,
    GAME_OVER
}GamePhase;
 
typedef struct Game{
    Board board;
    Dice dice;
    float turnTimer;        // timer tra le fasi
    int turn;               // 0 = AI_1 (bianco), 1 = AI_2 (nero)
    bool paused;            // stoppa AI
    bool running;           // gioco avviato
    GamePhase phase;        // fase corrente del gioco
    PlayerColor currentPlayer;
}Game;
 
void UpdateGame(Game *game);