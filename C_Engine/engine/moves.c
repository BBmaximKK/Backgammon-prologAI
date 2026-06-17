#include "moves.h"
#include <stdio.h>
#include <stdlib.h>


// ─────────────────────────────────────────────────────────────────
// CanBearOff
//
// Il giocatore può fare bear-off solo se TUTTE le sue pedine
// si trovano nel proprio home board (ultime 6 posizioni).
//
// Home board BIANCO: punti 0..5
// Home board NERO:   punti 18..23
// ─────────────────────────────────────────────────────────────────
int CanBearOff(Board *b, PlayerColor player){
    if(player == PLAYER_WHITE){
        if(b->barWhite > 0)         return 0;
        // nessuna pedina bianca fuori dall'home board (punti 6..23)
        for(int i = 6; i < 24; i++){
            if(b->points[i] > 0)    return 0;
        }
        return 1;
    }else{
        if(b->barBlack > 0)         return 0;
        // nessuna pedina nera fuori dall'home board (punti 0..17)
        for(int i = 0; i < 18; i++){
            if(b->points[i] < 0)    return 0;
        }
        return 1;
    }
}

// ─────────────────────────────────────────────────────────────────
// Registra l'ultima mossa per l'highlight grafico
// ─────────────────────────────────────────────────────────────────
static void RecordMove(Board *b, int from, int to){
    if(b->lastMovedCount < 4){
        b->lastMovedFrom[b->lastMovedCount] = from;
        b->lastMovedTo[b->lastMovedCount]   = to;
        b->lastMovedCount++;
    }
}

// ─────────────────────────────────────────────────────────────────
// MoveChecker
//
// points[i] > 0 → pedine BIANCHE   (si muovono verso indici più bassi)
// points[i] < 0 → pedine NERE      (si muovono verso indici più alti)
//
// Ritorna 1 se la mossa è stata applicata, 0 se illegale.
// ─────────────────────────────────────────────────────────────────
int MoveChecker(Board *b, int from, int to, PlayerColor player){

    // ── 1. RE-ENTRY DAL BAR ──────────────────────────────────────
    if(player == PLAYER_WHITE && b->barWhite > 0){
        if(from != IDX_BAR_WHITE) return 0;
        if(to < 18 || to > 23)    return 0;

        int dst = b->points[to];
        if(dst < -1) return 0;

        if(dst == -1){
            b->points[to] = 0;
            b->barBlack++;
            b->pipBlack += (24 - to);   // pedina nera mangiata torna al bar
        }
        b->points[to]++;
        b->barWhite--;
        b->pipWhite -= (24 - to);       // distanza percorsa dal bar al campo to
        RecordMove(b, from, to);
        printf("RE-ENTRY BIANCO bar -> %d\n", to);
        return 1;
    }

    if(player == PLAYER_BLACK && b->barBlack > 0){
        if(from != IDX_BAR_BLACK) return 0;
        if(to < 0 || to > 5)      return 0;

        int dst = b->points[to];
        if(dst > 1) return 0;

        if(dst == 1){
            b->points[to] = 0;
            b->barWhite++;
            b->pipWhite += (to + 1);    // pedina bianca mangiata torna al bar
        }
        b->points[to]--;
        b->barBlack--;
        b->pipBlack -= (to + 1);        // distanza percorsa dal bar al campo to
        RecordMove(b, from, to);
        printf("RE-ENTRY NERO bar -> %d\n", to);
        return 1;
    }
 
    // ── 2. BEAR-OFF ──────────────────────────────────────────────
    if(to == IDX_BEAROFF_W || to == IDX_BEAROFF_B){
        if(!CanBearOff(b, player)) return 0;
 
        if(player == PLAYER_WHITE){
            if(to != IDX_BEAROFF_W) return 0;
            if(from < 0 || from > 5) return 0;
            if(b->points[from] <= 0) return 0;
            b->points[from]--;
            b->bearoffWhite++;
            b->pipWhite -= (from + 1);   // distanza percorsa (1-based)
        } else {
            if(to != IDX_BEAROFF_B) return 0;
            if(from < 18 || from > 23) return 0;
            if(b->points[from] >= 0)   return 0;
            b->points[from]++;
            b->bearoffBlack++;
            b->pipBlack -= (24 - from);  // distanza percorsa
        }
        RecordMove(b, from, to);
        printf("BEAR-OFF %s: %d\n", player == PLAYER_WHITE ? "BIANCO" : "NERO", from);
        return 1;
    }
 
    // ── 3. MOSSA NORMALE ─────────────────────────────────────────
    if(from < 0 || from >= FIELD) return 0;
    if(to   < 0 || to   >= FIELD) return 0;
 
    int src = b->points[from];
    if(src == 0) return 0;
 
    // verifica colore corretto
    if(player == PLAYER_WHITE && src < 0) return 0;
    if(player == PLAYER_BLACK && src > 0) return 0;
 
    int dst = b->points[to];
 
    if(src > 0){
        // bianco: muove verso indici bassi
        if(to >= from)  return 0;   // direzione sbagliata
        if(dst < -1)    return 0;   // bloccato da >= 2 nere
    } else {
        // nero: muove verso indici alti
        if(to <= from)  return 0;   // direzione sbagliata
        if(dst > 1)     return 0;   // bloccato da >= 2 bianche
    }
 
    // rimuove pedina dalla sorgente
    if(src > 0) b->points[from]--;
    else        b->points[from]++;
 
    // mangiata
    if(src > 0 && dst == -1){
        b->points[to] = 0;
        b->barBlack++;
        printf("MANGIATA: pedina nera sul bar (punto %d)\n", to);
    } else if(src < 0 && dst == 1){
        b->points[to] = 0;
        b->barWhite++;
        printf("MANGIATA: pedina bianca sul bar (punto %d)\n", to);
    }
 
    // aggiunge pedina alla destinazione
    if(src > 0) b->points[to]++;
    else        b->points[to]--;

    // aggiorna pip count:
    // bianco si muove da from verso to (indici decrescenti): pip -= (from - to)
    // nero si muove da from verso to (indici crescenti):     pip -= (to - from)
    // nel caso di mangiata, la pedina avversaria torna al bar: pip avversario aumenta
    if(src > 0){
        b->pipWhite -= (from - to);
        if(dst == -1) b->pipBlack += (24 - to);   // pedina nera mangiata torna al bar
    } else {
        b->pipBlack -= (to - from);
        if(dst == 1)  b->pipWhite += (to + 1);    // pedina bianca mangiata torna al bar
    }

    RecordMove(b, from, to);
    return 1;
}

//wrapper logico per la funzione MoveChecker
void ApplyMove(Board *b, Move *m, PlayerColor player){
    printf("ApplyMove: %d -> %d\n", m->from, m->to);
    MoveChecker(b, m->from, m->to, player);
}