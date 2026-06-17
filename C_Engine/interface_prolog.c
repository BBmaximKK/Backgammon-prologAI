#include <SWI-Prolog.h>
#include <stdio.h>
#include <string.h>
#include "interface_prolog.h"

static int prolog_ready = 0;

// ─────────────────────────────────────────────────────────────────
// BoardToPrologString
// ─────────────────────────────────────────────────────────────────
static void BoardToPrologString(Board *b, char *out){
    char buffer[32];
    out[0] = '[';
    out[1] = '\0';

    for(int i = 0; i < 24; i++){
        sprintf(buffer, "%d", b->points[i]);
        strcat(out, buffer);
        if(i != 23) strcat(out, ",");
    }
    strcat(out, "]");
}

// ─────────────────────────────────────────────────────────────────
// Prolog_Init
// ─────────────────────────────────────────────────────────────────
void Prolog_Init(){
    if(prolog_ready) return;

    char *argv[] = {"swipl", "-q", "-f", "ai.pl", NULL};
    int argc = 3;

    if(!PL_initialise(argc, argv)){
        printf("Errore avvio Prolog\n");
        return;
    }

    prolog_ready = 1;
    printf("Prolog avviato!\n");
}

// ─────────────────────────────────────────────────────────────────
// Prolog_Close
// ─────────────────────────────────────────────────────────────────
void Prolog_Close(){
    if(prolog_ready)
        PL_halt(0);
}

// ─────────────────────────────────────────────────────────────────
// Prolog_GetMove
// ─────────────────────────────────────────────────────────────────
void Prolog_GetMove(Board *board, Dice *dice, Move *move){
    char boardStr[512];
    BoardToPrologString(board, boardStr);

    predicate_t pred = PL_predicate("choose_move", 5, NULL);
    term_t args = PL_new_term_refs(5);

    (void)PL_put_atom_chars(args+0, boardStr);
    (void)PL_put_integer(args+1, dice->d1);
    (void)PL_put_integer(args+2, dice->d2);

    qid_t q = PL_open_query(NULL, PL_Q_NORMAL, pred, args);

    if(PL_next_solution(q)){
        (void)PL_get_integer(args+3, &move->from);
        (void)PL_get_integer(args+4, &move->to);
    } else {
        printf("Nessuna mossa trovata da Prolog\n");
        move->from = -1;
        move->to   = -1;
    }

    PL_close_query(q);
}

// ─────────────────────────────────────────────────────────────────
// Prolog_GetTop5
// ─────────────────────────────────────────────────────────────────
void Prolog_GetTop5(Board *board, Dice *dice, int player){

    // azzera il risultato precedente
    board->topMovesCount = 0;
    for(int i = 0; i < TOP_MOVES_N; i++){
        board->topMoves[i].from  = -1;
        board->topMoves[i].to    = -1;
        board->topMoves[i].score = 0.0f;
    }

    if(!prolog_ready) return;

    char pointsBuf[256];
    BoardToPrologString(board, pointsBuf);

    const char *playerAtom = (player == 1) ? "white" : "black";

    // ── board(Points, BarW, BarB, OffW, OffB, Turn) ──────────────
    term_t boardArgs = PL_new_term_refs(6);
    term_t pointsList = PL_new_term_ref();
    PL_chars_to_term(pointsBuf, &pointsList);
    PL_put_term(boardArgs+0, pointsList);
    PL_put_integer(boardArgs+1, board->barWhite);
    PL_put_integer(boardArgs+2, board->barBlack);
    PL_put_integer(boardArgs+3, board->bearoffWhite);
    PL_put_integer(boardArgs+4, board->bearoffBlack);
    PL_put_atom_chars(boardArgs+5, playerAtom);

    term_t boardTerm = PL_new_term_ref();
    PL_cons_functor_v(boardTerm,
                      PL_new_functor(PL_new_atom("board"), 6),
                      boardArgs);

    // ── dice(D1, D2) ─────────────────────────────────────────────
    term_t diceArgs = PL_new_term_refs(2);
    PL_put_integer(diceArgs+0, dice->d1);
    PL_put_integer(diceArgs+1, dice->d2);

    term_t diceTerm = PL_new_term_ref();
    PL_cons_functor_v(diceTerm,
                      PL_new_functor(PL_new_atom("dice"), 2),
                      diceArgs);

    // ── chiama top5_moves(Board, Dice, Player, Top5) ─────────────
    predicate_t pred = PL_predicate("top5_moves", 4, "ai");
    term_t args = PL_new_term_refs(4);
    PL_put_term(args+0, boardTerm);
    PL_put_term(args+1, diceTerm);
    PL_put_atom_chars(args+2, playerAtom);
    PL_put_variable(args+3);    // output

    qid_t q = PL_open_query(NULL, PL_Q_NORMAL, pred, args);
    if(!PL_next_solution(q)){
        PL_close_query(q);
        return;
    }

    // ── itera lista [move(F,T,S), ...] ───────────────────────────
    term_t tail = PL_copy_term_ref(args+3);
    term_t head = PL_new_term_ref();
    int idx = 0;

    while(idx < TOP_MOVES_N && PL_get_list(tail, head, tail)){
        term_t f = PL_new_term_ref();
        term_t t = PL_new_term_ref();
        term_t s = PL_new_term_ref();
        PL_get_arg(1, head, f);
        PL_get_arg(2, head, t);
        PL_get_arg(3, head, s);

        int from = -1, to = -1;
        double score = 0.0;
        PL_get_integer(f, &from);
        PL_get_integer(t, &to);
        PL_get_float(s, &score);

        board->topMoves[idx].from  = from;
        board->topMoves[idx].to    = to;
        board->topMoves[idx].score = (float)score;
        idx++;
    }
    board->topMovesCount = idx;

    PL_close_query(q);
}