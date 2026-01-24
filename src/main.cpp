#include "samples/collecting_game.h"
#include "samples/fighting_game.h"
#include "samples/zombie_game.h"

// Emscripten is used for web builds.
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

Game game;

void update()
{
    float delta_time = GetFrameTime();
    game.update(delta_time);
}

// TODO: Make this a GUI app for each platform.
int main(int argc, char** argv)
{
    // Initialize the window
    game.add_manager<WindowManager>(1280, 720, "Game Jam Kit");
    auto font_manager = game.add_manager<FontManager>();
    game.init();

    // Game::init initializes all managers, so we can load fonts now.
    font_manager->load_font("Roboto", "assets/fonts/Roboto.ttf", 64);
    font_manager->load_font("Tiny5", "assets/fonts/Tiny5.ttf", 64);
    font_manager->set_texture_filter("Roboto", TEXTURE_FILTER_BILINEAR);
    game.add_scene<ZombieScene>("zombie");
    game.add_scene<CollectingScene>("collecting");
    game.add_scene<FightingScene>("fighting");

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
