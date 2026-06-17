//setup dado
#pragma once

typedef struct{
    int d1;
    int d2;
    int isDouble;   // 1 se e doppio
    int rolls[4];   // i 4 valori da usare (o 2 se non doppio)
    int rollsLeft;  // quante mosse rimangono da fare
} Dice;

void RollDice(Dice *d);