#include "ai.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ─────────────────────────────────────────────────────────────────
// ScoreMove — valutazione euristica di una singola mossa
//   +distanza percorsa (avanzamento pip)
//   +50 se colpiamo una pedina avversaria
//   +30 se copriamo un nostro blot
//   +20 se occupiamo un campo vuoto (nuovo punto)
//   -40 se lasciamo un blot (partiamo da campo con 2 pedine)
// ─────────────────────────────────────────────────────────────────
static float ScoreMove(Board *b, int from, int to, PlayerColor player){
    float score = 0.0f;

    // avanzamento pip
    if(from == IDX_BAR_WHITE || from == IDX_BAR_BLACK)
        score += 10.0f;
    else if(to == IDX_BEAROFF_W || to == IDX_BEAROFF_B)
        score += 25.0f;
    else
        score += (float)abs(to - from);

    if(to < 0 || to >= FIELD) return score;

    int dst = b->points[to];

    // colpo avversario
    if(player == PLAYER_WHITE && dst == -1) score += 50.0f;
    if(player == PLAYER_BLACK && dst ==  1) score += 50.0f;

    // copriamo un nostro blot (campo da 1 a 2)
    if(player == PLAYER_WHITE && dst ==  1) score += 30.0f;
    if(player == PLAYER_BLACK && dst == -1) score += 30.0f;

    // campo vuoto → nuovo punto
    if(dst == 0) score += 20.0f;

    // lasciamo un blot partendo da campo con 2 pedine
    if(from >= 0 && from < FIELD){
        int src = b->points[from];
        if(player == PLAYER_WHITE && src == 2) score -= 40.0f;
        if(player == PLAYER_BLACK && src == -2) score -= 40.0f;
    }

    return score;
}

// ─────────────────────────────────────────────────────────────────
// CollectTop5 — raccoglie le top5 mosse candidate per il dado val
//   Salva in board->topMoves[] le mosse ordinate per score
// ─────────────────────────────────────────────────────────────────
static void CollectTop5(Board *b, int val, PlayerColor player){
    // candidati: array temporaneo
    TopMove cands[24];
    int     nc = 0;

    // ── re-entry dal bar ─────────────────────────────────────────
    if(player == PLAYER_WHITE && b->barWhite > 0){
        int to = 24 - val;
        if(to >= 18 && to <= 23 && b->points[to] >= -1){
            float s = ScoreMove(b, IDX_BAR_WHITE, to, player);
            cands[nc++] = (TopMove){IDX_BAR_WHITE, to, s};
        }
    } else if(player == PLAYER_BLACK && b->barBlack > 0){
        int to = val - 1;
        if(to >= 0 && to <= 5 && b->points[to] <= 1){
            float s = ScoreMove(b, IDX_BAR_BLACK, to, player);
            cands[nc++] = (TopMove){IDX_BAR_BLACK, to, s};
        }
    } else if(CanBearOff(b, player)){
        // ── bear-off ─────────────────────────────────────────────
        if(player == PLAYER_WHITE){
            for(int i = 5; i >= 0; i--){
                if(b->points[i] > 0 && val >= i + 1){
                    float s = ScoreMove(b, i, IDX_BEAROFF_W, player);
                    cands[nc++] = (TopMove){i, IDX_BEAROFF_W, s};
                    break;
                }
            }
        } else {
            for(int i = 18; i <= 23; i++){
                if(b->points[i] < 0 && val >= 24 - i){
                    float s = ScoreMove(b, i, IDX_BEAROFF_B, player);
                    cands[nc++] = (TopMove){i, IDX_BEAROFF_B, s};
                    break;
                }
            }
        }
        // anche le mosse normali in casa (potrebbero essere migliori)
        if(player == PLAYER_WHITE){
            for(int i = 5; i >= 0 && nc < 24; i--){
                if(b->points[i] > 0){
                    int to = i - val;
                    if(to >= 0 && b->points[to] >= -1){
                        float s = ScoreMove(b, i, to, player);
                        cands[nc++] = (TopMove){i, to, s};
                    }
                }
            }
        } else {
            for(int i = 18; i <= 23 && nc < 24; i++){
                if(b->points[i] < 0){
                    int to = i + val;
                    if(to <= 23 && b->points[to] <= 1){
                        float s = ScoreMove(b, i, to, player);
                        cands[nc++] = (TopMove){i, to, s};
                    }
                }
            }
        }
    } else {
        // ── mosse normali ─────────────────────────────────────────
        if(player == PLAYER_WHITE){
            for(int i = 23; i >= 0 && nc < 24; i--){
                if(b->points[i] > 0){
                    int to = i - val;
                    if(to >= 0 && b->points[to] >= -1){
                        float s = ScoreMove(b, i, to, player);
                        cands[nc++] = (TopMove){i, to, s};
                    }
                }
            }
        } else {
            for(int i = 0; i < 24 && nc < 24; i++){
                if(b->points[i] < 0){
                    int to = i + val;
                    if(to <= 23 && b->points[to] <= 1){
                        float s = ScoreMove(b, i, to, player);
                        cands[nc++] = (TopMove){i, to, s};
                    }
                }
            }
        }
    }

    // ordina per score decrescente (insertion sort, nc piccolo)
    for(int i = 1; i < nc; i++){
        TopMove key = cands[i];
        int j = i - 1;
        while(j >= 0 && cands[j].score < key.score){
            cands[j+1] = cands[j];
            j--;
        }
        cands[j+1] = key;
    }

    // salva top5
    b->topMovesCount = nc < TOP_MOVES_N ? nc : TOP_MOVES_N;
    for(int i = 0; i < b->topMovesCount; i++)
        b->topMoves[i] = cands[i];
    // azzera le slot rimanenti
    for(int i = b->topMovesCount; i < TOP_MOVES_N; i++)
        b->topMoves[i] = (TopMove){-1, -1, 0.0f};
}

// ─────────────────────────────────────────────────────────────────
// AI_PlayTurn
// ─────────────────────────────────────────────────────────────────
void AI_PlayTurn(Board *b, Dice d, PlayerColor player){

    b->lastMovedCount = 0;
    for(int i = 0; i < 4; i++){
        b->lastMovedFrom[i] = -1;
        b->lastMovedTo[i]   = -1;
    }

    int totalMoves = d.rollsLeft;

    for(int di = 0; di < totalMoves; di++){
        int val   = d.rolls[di];
        int moved = 0;

        // calcola top5 per questo dado prima di muovere
        CollectTop5(b, val, player);

        // ── RE-ENTRY DAL BAR ──────────────────────────────────────
        if(player == PLAYER_WHITE && b->barWhite > 0){
            int to = 24 - val;
            if(MoveChecker(b, IDX_BAR_WHITE, to, player)){
                printf("AI BIANCO re-entry: bar -> %d (dado %d)\n", to, val);
                moved = 1;
            }
            if(!moved)
                printf("AI BIANCO: nessun re-entry valido per dado %d\n", val);
            continue;
        }

        if(player == PLAYER_BLACK && b->barBlack > 0){
            int to = val - 1;
            if(MoveChecker(b, IDX_BAR_BLACK, to, player)){
                printf("AI NERO re-entry: bar -> %d (dado %d)\n", to, val);
                moved = 1;
            }
            if(!moved)
                printf("AI NERO: nessun re-entry valido per dado %d\n", val);
            continue;
        }

        // ── BEAR-OFF ──────────────────────────────────────────────
        if(CanBearOff(b, player)){
            if(player == PLAYER_WHITE){
                int exact = val - 1;
                if(exact >= 0 && exact <= 5 && b->points[exact] > 0){
                    if(MoveChecker(b, exact, IDX_BEAROFF_W, player)){
                        printf("AI BIANCO bear-off esatto: punto %d (dado %d)\n", exact, val);
                        moved = 1;
                    }
                }
                if(!moved){
                    for(int i = 5; i >= 0 && !moved; i--){
                        if(b->points[i] > 0){
                            if(val >= i + 1){
                                if(MoveChecker(b, i, IDX_BEAROFF_W, player)){
                                    printf("AI BIANCO bear-off eccedente: punto %d (dado %d)\n", i, val);
                                    moved = 1;
                                }
                            }
                            break;
                        }
                    }
                }
            } else {
                int exact = 24 - val;
                if(exact >= 18 && exact <= 23 && b->points[exact] < 0){
                    if(MoveChecker(b, exact, IDX_BEAROFF_B, player)){
                        printf("AI NERO bear-off esatto: punto %d (dado %d)\n", exact, val);
                        moved = 1;
                    }
                }
                if(!moved){
                    for(int i = 18; i <= 23 && !moved; i++){
                        if(b->points[i] < 0){
                            if(val >= 24 - i){
                                if(MoveChecker(b, i, IDX_BEAROFF_B, player)){
                                    printf("AI NERO bear-off eccedente: punto %d (dado %d)\n", i, val);
                                    moved = 1;
                                }
                            }
                            break;
                        }
                    }
                }
            }
            if(moved) continue;
        }

        // ── MOSSA NORMALE ─────────────────────────────────────────
        if(player == PLAYER_WHITE){
            for(int i = 23; i >= 0 && !moved; i--){
                if(b->points[i] > 0){
                    int to = i - val;
                    if(MoveChecker(b, i, to, player)){
                        printf("AI BIANCO: %d -> %d (dado %d)\n", i, to, val);
                        moved = 1;
                    }
                }
            }
        } else {
            for(int i = 0; i < 24 && !moved; i++){
                if(b->points[i] < 0){
                    int to = i + val;
                    if(MoveChecker(b, i, to, player)){
                        printf("AI NERO: %d -> %d (dado %d)\n", i, to, val);
                        moved = 1;
                    }
                }
            }
        }

        if(!moved)
            printf("AI: nessuna mossa valida per dado %d\n", val);
    }
}