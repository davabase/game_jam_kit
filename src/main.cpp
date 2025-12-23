#include <iostream>
#include <thread>

#include <raylib.h>
#include "scene.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

Playground scene;

void update() {
    float delta_time = GetFrameTime();
    scene.update(delta_time);

    // Start drawing
    BeginDrawing();

    ClearBackground(RAYWHITE);
    // Draw the scene
    scene.draw();

    // End drawing
    EndDrawing();
}

int main(int argc, char** argv) {
    // Initialize the window
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Game Jam Kit");
    SetTargetFPS(60);

    scene.init();

    // Main game loop
    #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(update, 0, true);
    #else
    while (!WindowShouldClose()) {
        update();
    }
    #endif
    return 0;
}
