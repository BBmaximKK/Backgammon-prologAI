#pragma once

#include "graphics/state.h"
#include "engine/dice.h"
#include "move.h"

void Prolog_Init();
void Prolog_Close();
void Prolog_GetMove(Board *board, Dice *dice, Move *move);
void Prolog_GetTop5(Board *board, Dice *dice, int player);