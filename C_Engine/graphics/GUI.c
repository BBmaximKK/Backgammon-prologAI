#include "render.h"
#include "../game.h"
#include <stdio.h>

//POSIZIONE E DIMENIONE PANELLO
#define WIN_W      1150
#define WIN_H      700
#define BOARD_W    830
#define PANEL_X    BOARD_W
#define PANEL_W    (WIN_W - BOARD_W)   // 320
#define PANEL_H    WIN_H

//GEOMETRIA TASTI
#define BTN_W 120
#define BTN_H 44

//ALTEZZE SEZZIONI(somma = PANEL_H)
#define SEC_START_H   90     // bottone START
#define SEC_PIP_H     60     // PIP bianco / nero
#define SEC_MOVES_H   160    // top 5 mosse
#define SEC_DICE_H    120    // dadi  
#define SEC_FIRST_H   50     // chi ha fatto la prima mossa
#define SEC_SCORE_H   100    // punteggio
#define SEC_NEXT_H    (PANEL_H - SEC_START_H - SEC_PIP_H - SEC_MOVES_H -  SEC_DICE_H - SEC_FIRST_H - SEC_SCORE_H)


//Y DI OGNI SEZIONE
#define Y_START  0
#define Y_PIP    (Y_START + SEC_START_H)
#define Y_MOVES  (Y_PIP   + SEC_PIP_H)
#define Y_DICE   (Y_MOVES + SEC_MOVES_H)
#define Y_FIRST  (Y_DICE + SEC_DICE_H)
#define Y_SCORE  (Y_FIRST + SEC_FIRST_H)
#define Y_NEXT   (Y_SCORE + SEC_NEXT_H)   

//COLORI PANELLI
#define COL_BG      (Color){245,245,240,255}
#define COL_LINE    (Color){180,180,180,255}
#define COL_TXT     (Color){30,30,30,255}
#define COL_BTN     (Color){60,120,60,255}
#define COL_BTN_R   (Color){194,47,47,255}
#define COL_DICE    (Color){230,230,230,255}
#define COL_BTN_TXT RAYWHITE

//riga separatrice
static void HLine(int y){
    DrawLine(PANEL_X, y, PANEL_X + PANEL_W, y, COL_LINE);
}

// START: metà sinistra del pannello
static Rectangle StartBtnRect(void){
    int bx = PANEL_X + (PANEL_W - BTN_W) / 8;
    int by = Y_START + (SEC_START_H - BTN_H) / 2;
    return (Rectangle){bx, by, BTN_W, BTN_H};
}

// RESET: meta destra del pannello
static Rectangle ResetBtnRect(void){
    int bx = PANEL_X + (PANEL_W - BTN_W) / 2 + PANEL_W / 4;
    int by = Y_START + (SEC_START_H - BTN_H) / 2;
    return (Rectangle){bx, by, BTN_W, BTN_H};
}

// STOP: sezione bassa, in centro del pannello
static Rectangle StopBtnRect(void){
    int btnsW = 50, btnsH = 50;
    int bx = PANEL_X + (PANEL_W - btnsW) / 2;
    int by = Y_NEXT  + (SEC_NEXT_H - btnsH) / 2;
    return (Rectangle){bx, by, btnsW, btnsH};
}

//Funzione per la gestione dei click
//Chiamata dal main
void HandleClickPanel(Game *g, Vector2 mouse){

    // START
    if(CheckCollisionPointRec(mouse, StartBtnRect())){
        if(!g->running){
            //primo click avvia il gioco
            g->running = true;
            g->paused = false;
            g->turn = 1;

            g->currentPlayer = PLAYER_WHITE;    //random in futuro 
            g->phase = GAME_ROLL_DICE;
            g->board.gameStarted = true;
        }
        // se già running, START non fa nulla (usa STOP per fermare)
        return;
    }

    //RESET
    if(CheckCollisionPointRec(mouse, ResetBtnRect())){
        g->running = false;
        g->paused = false;
        g->turn = 0;
        printf("\n---------RESERT---------\n");
        InitBoard(&g->board);   //riporta la tavola allo stato iniziale
        return;
    }

    //STOP
    if(CheckCollisionPointRec(mouse, StopBtnRect())){
        if(!g->running) return;     //gioco ancora non avviato -> igniora
        g->paused = !g->paused;     //toggle: ferma <-> riprendi
        printf("\n---------PAUSED---------\n");
        return;
    }
}

//disegna il panello 
void DrawPanel(Board *b, Game *g){
    //sfondo panello
    DrawRectangle(PANEL_X, 0, PANEL_W, PANEL_H, COL_BG);

    //SEZIONE START E RESET
    {
        Rectangle sr = StartBtnRect();
        Rectangle rr = ResetBtnRect();

        // colore START cambia in base allo stato
        Color startCol = COL_BTN;
        const char *startLabel = "START";
        if(g->running){
            startCol  = DARKGREEN;
            startLabel = "IN CORSO";
        }

        DrawRectangleRounded(sr, 0.25f, 8, startCol);
        DrawText(startLabel, sr.x + ((startLabel == "START") ? 25 : 12), sr.y + 12, 20, COL_BTN_TXT);

        DrawRectangleRounded(rr, 0.25f, 8, COL_BTN_R);
        DrawText("RESET", rr.x + 20, rr.y + 12, 24, COL_BTN_TXT);
    }

    //separatore
    HLine(Y_PIP);

    //SEZIONE PIP
    {
        int y = Y_PIP + 10;
        char buf[32];

        //etichette
        DrawText("PIP Bianco", PANEL_X + 14, y, 16, COL_TXT);
        DrawText("PIP Nero",   PANEL_X + PANEL_W/2 + 14, y, 16, COL_TXT);

        //stampo valori
        sprintf(buf, "%d", b->pipWhite);
        DrawText(buf, PANEL_X + 14, y + 22, 20, DARKBLUE);
        sprintf(buf, "%d", b->pipBlack);
        DrawText(buf, PANEL_X + PANEL_W/2 + 14, y + 22, 20, DARKBLUE);

        //linea cetrale verticale
        DrawLine(PANEL_X + PANEL_W/2, Y_PIP, PANEL_X + PANEL_W/2, Y_MOVES, COL_LINE);
    }

    //separatore
    HLine(Y_MOVES);

    //SEZIONE MOSSE
    {
        DrawText("Top 5 Mosse consigliate", PANEL_X + 10, Y_MOVES + 8, 15, COL_TXT);
        DrawText("(valore evaluation)", PANEL_X + 10, Y_MOVES + 26, 13, GRAY);

        for(int i = 0; i < 5; i++){
            int ry = Y_MOVES + 50 + i * 22;

            if(!g->running || i >= b->topMovesCount || b->topMoves[i].from == -1){
                DrawText("--", PANEL_X + 14, ry, 14, LIGHTGRAY);
                continue;
            }

            TopMove *tm = &b->topMoves[i];

            // colore giocatore corrente
            const char *colLetter = (g->currentPlayer == PLAYER_WHITE) ? "b" : "n";

            // from leggibile (1-based per display, bar=BAR, off=OUT)
            char fromStr[8], toStr[8];
            if(tm->from == IDX_BAR_WHITE || tm->from == IDX_BAR_BLACK)
                snprintf(fromStr, sizeof(fromStr), "BAR");
            else
                snprintf(fromStr, sizeof(fromStr), "%d", tm->from + 1);

            if(tm->to == IDX_BEAROFF_W || tm->to == IDX_BEAROFF_B)
                snprintf(toStr, sizeof(toStr), "OUT");
            else
                snprintf(toStr, sizeof(toStr), "%d", tm->to + 1);

            char buf[48];
            snprintf(buf, sizeof(buf), "%s:%s->%s  (%.1f)",
                     colLetter, fromStr, toStr, tm->score);

            // mossa migliore in verde, resto grigio scuro
            Color c = (i == 0) ? DARKGREEN : (Color){80, 80, 80, 255};
            DrawText(buf, PANEL_X + 14, ry, 13, c);
        }
    }

    //separatore
    HLine(Y_DICE);

    //SEZIONE DADI
    {
        int dW = 80, dH = 80;
        int dx = PANEL_X + (PANEL_W - dW) / 4;
        int dy = Y_DICE + (SEC_DICE_H - dH) / 2;

        //dado destro
        DrawRectangle(dx-2, dy-2, dW+4, dH+4, DARKGRAY);
        DrawRectangle(dx, dy, dW, dH, COL_DICE);

        //dado sinistro
        int dxs = PANEL_X + PANEL_W / 2 + (PANEL_W - dW) / 8;
        DrawRectangle(dxs-2, dy-2, dW+4, dH+4, DARKGRAY);
        DrawRectangle(dxs, dy, dW, dH, COL_DICE);

        //mostra i valori del daod se il gioco e avviato
        if(g->running){
            char buf[4];
            //PRIMO dado
            sprintf(buf, "%d", g->dice.d1);
            DrawText(buf, dx + 28, dy + 25, 30, DARKGRAY);

            //SECONDO dado
            sprintf(buf, "%d", g->dice.d2);
            DrawText(buf, dxs + 28, dy + 25, 30, DARKGRAY);
        }
    }

    //separatore
    HLine(Y_FIRST);

    //SEZIONE PRIMA MOSSA
    {
        int y = Y_FIRST + 12;
        const char *msg = "Chi ha fatto la prima mossa: --";
        if(b->firstMover ==  1) msg = "Prima mossa: BIANCHI";
        if(b->firstMover == -1) msg = "Prima mossa: NERI";
        DrawText(msg, PANEL_X + 10, y, 14, COL_TXT);
    }

    //separatore
    HLine(Y_SCORE);

    //SEZIONE PUNTTEGGIO
    {
        char buf[32];
        int y = Y_SCORE + 10;

        // linea verticale centrale
        DrawLine(PANEL_X + PANEL_W/2, Y_SCORE, PANEL_X + PANEL_W/2, Y_NEXT, COL_LINE);

        DrawText("Punteggio\nBianchi", PANEL_X + 14, y, 16, COL_TXT);
        DrawText("Punteggio\nNeri",    PANEL_X + PANEL_W/2 + 14, y, 16, COL_TXT);

        //stampo valori
        sprintf(buf, "%d", b->scoreWhite);
        DrawText(buf, PANEL_X + 14, y + 46, 24, DARKBLUE);
        sprintf(buf, "%d", b->scoreBlack);
        DrawText(buf, PANEL_X + PANEL_W/2 + 14, y + 46, 24, DARKBLUE);
    }

    //separatore
    HLine(Y_NEXT);

    //SEZIONE PROSSIMO E STOP
    {
        int btnW = 40, btnH = 40;
        int by   = Y_NEXT + (SEC_NEXT_H - btnH) / 2;

        // bottone PRECEDENTE (freccia sinistra)
        int bx = PANEL_X + (PANEL_W - btnW) / 8;
        DrawRectangleRounded((Rectangle){bx, by, btnW, btnH}, 0.25f, 8, DARKBLUE);
        DrawText("<", bx + 10, by - 2, 50, COL_BTN_TXT);

        // bottone PROSSIMO (freccia destra)
        int bxn = PANEL_X + (PANEL_W - btnW) / 2 + PANEL_W / 4 + PANEL_W / 16;
        DrawRectangleRounded((Rectangle){bxn, by, btnW, btnH}, 0.25f, 8, DARKBLUE);
        DrawText(">", bxn + 15, by - 2, 50, COL_BTN_TXT);

        
        // STOP tasto
        Rectangle sr = StopBtnRect();
        Color stopCol = DARKBLUE;
        Color dotCol = COL_BTN_R;
        const  char *stopLabel = "0";
        if(g->paused){
            stopCol = COL_BTN_R;
            dotCol = RAYWHITE;
            stopLabel = "||";   //icona pausa
        }

        DrawRectangleRounded(sr, 0.25f, 8, stopCol);
        DrawText(stopLabel, sr.x + (g->paused ? 19 : 15), sr.y + 7, 40, dotCol);
    }
}