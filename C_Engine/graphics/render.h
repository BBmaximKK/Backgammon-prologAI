#ifndef RENDER_H
#define RENDER_H

#include "raylib.h"
#include "state.h"

//FILE DI COSTRUZIONE PER board.c
// forward declaration (evita dipendenza circolare con game.h)
typedef struct Game Game;

void DrawBoard(Board *b);   //board.c
void DrawPanel(Board *b, Game *g);   // GUI.c
void HandleClickPanel(Game *g, Vector2 mouse);  //gestisci il click sui tasti

#endif
