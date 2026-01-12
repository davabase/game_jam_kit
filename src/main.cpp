#include "samples/fighting_game.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

Game game;

void update()
{
    float delta_time = GetFrameTime();
    game.update(delta_time);
}

int main(int argc, char** argv)
{
    // Initialize the window
    const int screen_width = 512 * 4;
    const int screen_height = 256 * 4;
    InitWindow(screen_width, screen_height, "Game Jam Kit");
    InitAudioDevice();
    SetTargetFPS(60);

    auto font_manager = game.add_manager<FontManager>();
    // font_manager->load_font("Roboto", "assets/Roboto.ttf", 64);
    // font_manager->set_texture_filter("Roboto", TEXTURE_FILTER_BILINEAR);
    game.init();

    game.add_scene<Fighting>("fighting");

// Main game loop
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(update, 0, true);
#else
    while (!WindowShouldClose())
    {
        update();
    }
#endif
    return 0;
}
