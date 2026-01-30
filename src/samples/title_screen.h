#pragma once

#include "engine/prefabs/includes.h"

class TitleScreen : public Scene
{
public:
    Font font;
    std::string title = "Game Jam";
    void init() override
    {
        auto font_manager = game->get_manager<FontManager>();
        font = font_manager->get_font("Roboto");
    }

    void update(float delta_time) override
    {
        // Trigger scene change on Enter key or gamepad start button.
        if (IsKeyPressed(KEY_ENTER) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT))
        {
            game->go_to_scene_next();
        }
    }

    void draw() override
    {
        auto width = GetScreenWidth();
        auto height = GetScreenHeight();
        auto title_text_size = MeasureTextEx(font, title.c_str(), 64, 0);

        std::string subtitle = "Press Start or Enter to Switch Scenes";
        auto subtitle_text_size = MeasureTextEx(font, subtitle.c_str(), 32, 0);

        ClearBackground(SKYBLUE);
        DrawTextEx(font,
                   title.c_str(),
                   {(width - title_text_size.x) / 2, (height - title_text_size.y - 100) / 2},
                   64,
                   1,
                   WHITE);
        DrawTextEx(font,
                   subtitle.c_str(),
                   {(width - subtitle_text_size.x) / 2, (height - subtitle_text_size.y + 100) / 2},
                   32,
                   1,
                   WHITE);
    }
};