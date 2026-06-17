#include "render.h"
#include <stdio.h>


// ── dimensioni ────────────────────────────────────────────────────
#define WIDTH     1150
#define HEIGHT    700
#define PANEL_W   320
#define BOARD_W   (WIDTH - PANEL_W)   // 830
#define BOARD_H   HEIGHT
 
#define BEAROFF_W 40
#define BAR_W     BEAROFF_W           // 40  (uguale al bear-off)
#define TRI_W     62
#define PLAY_W    (12 * TRI_W + BAR_W)  // 784
#define PLAY_H    HEIGHT
#define BAR_X     (6 * TRI_W)           // 372
#define TRI_H     (PLAY_H / 2 - 30)
#define CHECKER_R (TRI_W / 2 - 3)       // 28
 
// ── colori pedine ────────────────────────────────────────────────
#define COL_WHITE_C       (Color){230,230,230,255}
#define COL_BLACK_C       (Color){36,36,36,255}
#define COL_OUTLINE       (Color){27,25,57,255}
#define COL_HIGHLIGHT     (Color){220,40,40,255}    // outline rosso (non più usato)
#define COL_WHITE_MOVED   (Color){255,182,210,255}  // rosa chiaro — bianche mosse
#define COL_BLACK_MOVED   (Color){90,30,120,255}    // viola scuro  — nere mosse

// ─────────────────────────────────────────────────────────────────
// ColX: bordo sinistro della colonna col (0..11)
// ─────────────────────────────────────────────────────────────────
static int ColX(int col){
    if(col < 6) return col * TRI_W;
    else        return BAR_X + BAR_W + (col - 6) * TRI_W;
}

// ─────────────────────────────────────────────────────────────────
// PointCol: indice board (0..23) → colonna (0..11)
// ─────────────────────────────────────────────────────────────────
static int PointCol(int idx){
    return (idx >= 12) ? (idx - 12) : (11 - idx);
}
 
// ─────────────────────────────────────────────────────────────────
// IsHighlighted: controlla se il punto `idx` è una destinazione
// di una delle ultime mosse (per colorare l'outline in rosso)
// ─────────────────────────────────────────────────────────────────
static int IsHighlighted(Board *b, int idx){
    for(int i = 0; i < b->lastMovedCount; i++){
        if(b->lastMovedTo[i] == idx) return 1;
    }
    return 0;
}

// ─────────────────────────────────────────────────────────────────
// DrawTriangleField
// ─────────────────────────────────────────────────────────────────
static void DrawTriangleField(int x, bool top, Color color){
    float x0 = (float)x;
    float x1 = (float)(x + TRI_W);
    float xm = (float)(x + TRI_W / 2);
    if(top){
        DrawTriangle((Vector2){xm,(float)TRI_H},
                     (Vector2){x1, 0.f},
                     (Vector2){x0, 0.f}, color);
    } else {
        DrawTriangle((Vector2){xm,(float)(HEIGHT - TRI_H)},
                     (Vector2){x0,(float)HEIGHT},
                     (Vector2){x1,(float)HEIGHT}, color);
    }
}

// ─────────────────────────────────────────────────────────────────
// DrawCheckers — legge direttamente da b->points[]
// points[idx] > 0 → bianche (riga superiore: top=true scende dall'alto)
// points[idx] < 0 → nere
// ─────────────────────────────────────────────────────────────────
static void DrawCheckers(int val, int x, bool top, int highlighted){
    if(val == 0) return;

    int count = val > 0 ? val : -val;
    int cx    = x + TRI_W / 2;

    Color baseColor = val > 0 ? COL_WHITE_C    : COL_BLACK_C;
    Color hlColor   = val > 0 ? COL_WHITE_MOVED : COL_BLACK_MOVED;
    Color textCol   = val > 0 ? COL_OUTLINE    : COL_WHITE_C;

    int draw = count > 6 ? 6 : count;

    for(int i = 0; i < draw; i++){
        int cy = top
            ? (CHECKER_R + i * CHECKER_R * 2)
            : (HEIGHT - CHECKER_R - i * CHECKER_R * 2);

        // la pedina in cima (ultima disegnata) diventa rosa/viola se highlighted
        Color c = (highlighted && i == draw - 1) ? hlColor : baseColor;
        DrawCircle(cx, cy, CHECKER_R, c);
        DrawCircleLines(cx, cy, CHECKER_R, COL_OUTLINE);

        // contatore se >6
        if(count > 6 && i == draw - 1){
            char buf[4];
            snprintf(buf, sizeof(buf), "%d", count);
            int tw = MeasureText(buf, 14);
            DrawText(buf, cx - tw / 2, cy - 7, 14, textCol);
        }
    }
}

// ─────────────────────────────────────────────────────────────────
// DrawBar — disegna le pedine sul bar (striscia centrale)
// Il bar è la colonna BAR_X di larghezza BAR_W.
// Bianche: metà superiore, Nere: metà inferiore
// ─────────────────────────────────────────────────────────────────
static void DrawBar(Board *b){
    int cx = BAR_X + BAR_W / 2;
 
    // ── Pedine BIANCHE sul bar (metà alta) ───────────────────────
    for(int i = 0; i < b->barWhite; i++){
        int cy = CHECKER_R + i * CHECKER_R * 2;
        DrawCircle(cx, cy, CHECKER_R, COL_WHITE_C);
        DrawCircleLines(cx, cy, CHECKER_R, COL_OUTLINE);
    }
 
    // ── Pedine NERE sul bar (metà bassa) ─────────────────────────
    for(int i = 0; i < b->barBlack; i++){
        int cy = HEIGHT - CHECKER_R - i * CHECKER_R * 2;
        DrawCircle(cx, cy, CHECKER_R, COL_BLACK_C);
        DrawCircleLines(cx, cy, CHECKER_R, COL_OUTLINE);
    }
 
    // etichette
    if(b->barWhite > 0){
        char buf[8];
        snprintf(buf, sizeof(buf), "W:%d", b->barWhite);
        DrawText(buf, BAR_X + 4, HEIGHT / 2 - 50, 12, RAYWHITE);
    }
    if(b->barBlack > 0){
        char buf[8];
        snprintf(buf, sizeof(buf), "B:%d", b->barBlack);
        DrawText(buf, BAR_X + 4, HEIGHT / 2 + 36, 12, RAYWHITE);
    }
}

// ─────────────────────────────────────────────────────────────────
// DrawBearOff — disegna i counter di pedine uscite sul lato destro
// ─────────────────────────────────────────────────────────────────
static void DrawBearOff(Board *b){
    int bx = PLAY_W;
 
    // Sfondo già disegnato in DrawBoard (DARKBROWN).
    // Mostriamo solo testo e mini-indicatori.
 
    // Bianche (in alto)
    if(b->bearoffWhite > 0){
        char buf[16];
        snprintf(buf, sizeof(buf), "W\n%d", b->bearoffWhite);
        DrawText(buf, bx + 6, 8, 14, RAYWHITE);
 
        // mini pile: un cerchio per ogni pedina (massimo 15)
        int drawn = b->bearoffWhite < 15 ? b->bearoffWhite : 15;
        for(int i = 0; i < drawn; i++){
            int cy = 50 + i * 14;
            DrawCircle(bx + BEAROFF_W / 2, cy, 6, COL_WHITE_C);
            DrawCircleLines(bx + BEAROFF_W / 2, cy, 6, COL_OUTLINE);
        }
    }
 
    // Nere (in basso)
    if(b->bearoffBlack > 0){
        char buf[16];
        snprintf(buf, sizeof(buf), "B\n%d", b->bearoffBlack);
        DrawText(buf, bx + 6, HEIGHT - 45, 14, RAYWHITE);
 
        int drawn = b->bearoffBlack < 15 ? b->bearoffBlack : 15;
        for(int i = 0; i < drawn; i++){
            int cy = HEIGHT - 50 - i * 14;
            DrawCircle(bx + BEAROFF_W / 2, cy, 6, COL_BLACK_C);
            DrawCircleLines(bx + BEAROFF_W / 2, cy, 6, COL_OUTLINE);
        }
    }
}
 
// ─────────────────────────────────────────────────────────────────
// DrawBoard
// ─────────────────────────────────────────────────────────────────
void DrawBoard(Board *b){
    // sfondo tavola
    DrawRectangle(0, 0, BOARD_W, BOARD_H, (Color){181,136,99,255});
 
    // riga superiore (indici 12..23) — triangoli puntano verso il basso
    for(int i = 12; i < 24; i++){
        Color col = (i % 2) ? DARKBROWN : BEIGE;
        int x = ColX(PointCol(i));
        DrawTriangleField(x, true, col);
        DrawCheckers(b->points[i], x, true, IsHighlighted(b, i));
    }
 
    // riga inferiore (indici 0..11) — triangoli puntano verso l'alto
    for(int i = 11; i >= 0; i--){
        Color col = (i % 2) ? DARKBROWN : BEIGE;
        int x = ColX(PointCol(i));
        DrawTriangleField(x, false, col);
        DrawCheckers(b->points[i], x, false, IsHighlighted(b, i));
    }
 
    // barra centrale
    DrawRectangle(BAR_X, 0, BAR_W, HEIGHT, DARKBROWN);
 
    // pedine sul bar
    DrawBar(b);
 
    // bear-off strip
    DrawRectangle(PLAY_W, 0, BEAROFF_W, HEIGHT, DARKBROWN);
    DrawBearOff(b);
 
    // linea separatrice tavola/pannello
    DrawLine(BOARD_W, 0, BOARD_W, HEIGHT, DARKGRAY);
}