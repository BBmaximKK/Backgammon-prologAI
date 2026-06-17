#include "raylib.h"
#include <time.h>
#include <stdlib.h>

#include "graphics/state.h"
#include "graphics/render.h"
#include "game.h"
#include "engine/ai.h"

#define WIDTH 1150
#define HEIGHT 700

//PER COMPILARE IL FILE
//gcc main.c engine/*.c graphics/*.c interface_prolog.c setup.c -Iengine -Igraphics -Iai -I"C:\Program Files\swipl\include" -L"C:\Program Files\swipl\bin" -lswipl -lws2_32 -lraylib -lopengl32 -lgdi32 -lwinmm -o backgammon.exe
// 

int main(void){
    InitWindow(WIDTH, HEIGHT, "Backgammon AI");
    SetTargetFPS(60);
    srand(time(NULL));

    Game game;
    game.running       = false;
    game.paused        = false;
    game.turn          = 0;
    game.phase         = GAME_ROLL_DICE;
    game.currentPlayer = PLAYER_WHITE;
    game.turnTimer     = 0.0f;

    InitBoard(&game.board);

    //ciclo affinche vivo
    //vive il programma
    while(!WindowShouldClose()){

        // ── INPUT ─────────────────────────────────────────────────
        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            Vector2 mouse = GetMousePosition();
            HandleClickPanel(&game, mouse); 
        }

        // ── LOGICA ────────────────────────────────────────────────
        UpdateGame(&game);
              
        // ── DRAW ──────────────────────────────────────────────────
        BeginDrawing();
            DrawBoard(&game.board);
            DrawPanel(&game.board, &game);
        EndDrawing();
    }

    // chiude la finestra
    // quando viene chiuso il programma
    CloseWindow();
    return 0;
}