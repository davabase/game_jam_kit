#include "scenes.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

Game game;

void update() {
    float delta_time = GetFrameTime();
    game.update(delta_time);
}

int main(int argc, char** argv) {
    // Initialize the window
    const int screen_width = 800;
    const int screen_height = 600;
    InitWindow(screen_width, screen_height, "Game Jam Kit");
    SetTargetFPS(60);

    auto font_manager = game.add_manager<FontManager>();
    font_manager->load_font("Roboto", "assets/Roboto.ttf", 64);
    font_manager->set_texture_filter("Roboto", TEXTURE_FILTER_BILINEAR);
    game.init();

    game.add_scene<Playground>("playground");
    game.add_scene<Playground>("playground2");

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
