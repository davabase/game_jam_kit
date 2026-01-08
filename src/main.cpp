#include "scenes.h"

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
    const int screen_width = 800;
    const int screen_height = 600;
    InitWindow(screen_width, screen_height, "Game Jam Kit");
    SetTargetFPS(60);

    auto manager_provider = std::make_unique<ManagerProvider>();
    auto font_manager = manager_provider->add_manager<FontManager>();
    font_manager->load_font("Roboto", "assets/Roboto.ttf", 64);
    font_manager->set_texture_filter("Roboto", TEXTURE_FILTER_BILINEAR);
    manager_provider->init();

    scene.init(manager_provider.get());

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
