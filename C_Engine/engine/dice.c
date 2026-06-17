#include <stdlib.h>
#include <time.h>

//passo la struttura del dado
#include "dice.h"

//Ottiene i valori dei due dadi
void RollDice(Dice *d){
    d -> d1 = rand() % 6 + 1;
    d -> d2 = rand() % 6 + 1;

    if(d->d1 == d->d2){
        d->isDouble = 1;
        d->rollsLeft = 4;
        d->rolls[0] = d->rolls[1] = d->rolls[2] = d->rolls[3] = d->d1;
    }else{
        d->isDouble = 0;
        d->rollsLeft = 2;
        d->rolls[0] = d->d1;
        d->rolls[1] = d->d2;
        d->rolls[2] = d->rolls[3] = 0;
    }
}