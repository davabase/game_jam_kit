#pragma once

#include "engine/prefabs/includes.h"

class FightingCharacter : public Character
{
public:
    AnimationController* animation;
    int player_number = 1;

    FightingCharacter(CharacterParams p, int player_number = 1) : Character(p), player_number(player_number) {}

    void init() override
    {
        Character::init();

        animation = add_component<AnimationController>(body);
        if (player_number == 1)
        {
            animation->add_animation("run",
                                     std::vector<std::string>{"assets/sunnyland/fox/run-1.png",
                                                              "assets/sunnyland/fox/run-2.png",
                                                              "assets/sunnyland/fox/run-3.png",
                                                              "assets/sunnyland/fox/run-4.png",
                                                              "assets/sunnyland/fox/run-5.png",
                                                              "assets/sunnyland/fox/run-6.png"},
                                     10.0f);
            animation->add_animation("idle",
                                     std::vector<std::string>{"assets/sunnyland/fox/idle-1.png",
                                                              "assets/sunnyland/fox/idle-2.png",
                                                              "assets/sunnyland/fox/idle-3.png",
                                                              "assets/sunnyland/fox/idle-4.png"},
                                     5.0f);
        }
        animation->set_scale(1.0f);
        animation->origin.y += 4;
    }

    void update(float delta_time) override
    {
        Character::update(delta_time);

        if (fabsf(movement->move_x) > 0.1f)
        {
            animation->play("run");
            animation->flip_x = movement->move_x < 0.0f;
        }
        else
        {
            animation->play("idle");
        }
    }

    void draw() override
    {
        // Character is drawn by the AnimationController.
    }
};

/**
 * A fighting game scene.
 */
class Fighting : public Scene
{
public:
    RenderTexture2D renderer;

    void init_services() override
    {
        add_service<TextureService>();
        add_service<SoundService>();
        add_service<PhysicsService>();
        std::vector<std::string> collision_names = {"walls"};
        add_service<LevelService>("assets/levels/fighting.ldtk", "Stage", collision_names, 1);
    }

    void init() override
    {
        auto level = get_service<LevelService>();
        auto player_entity = level->get_entities_by_name("Start")[2];
        if (!player_entity)
        {
            assert(false);
        }

        CharacterParams params;
        params.position = level->convert_to_pixels(player_entity->getPosition());
        params.width = 16;
        params.height = 24;
        auto character = add_game_object<FightingCharacter>(params);
        character->add_tag("character");

        auto camera = add_game_object<CameraObject>(Vector2{512, 256}, level->get_size());
        camera->add_tag("camera");

        renderer = LoadRenderTexture(512, 256);
    }

    void update(float delta_time) override
    {
        auto camera = static_cast<CameraObject*>(get_game_objects_with_tag("camera")[0]);
        auto player = static_cast<FightingCharacter*>(get_game_objects_with_tag("character")[0]);
        auto physics = get_service<PhysicsService>();
        camera->target = player->body->get_position_pixels();
    }

    void draw_scene() override
    {
        auto camera = static_cast<CameraObject*>(get_game_objects_with_tag("camera")[0]);
        auto physics = get_service<PhysicsService>();

        BeginTextureMode(renderer);
        ClearBackground(MAGENTA);
        camera->draw_begin();
        Scene::draw_scene();
        // physics->draw_debug();
        // camera->draw_debug();
        camera->draw_end();
        EndTextureMode();

        DrawTexturePro(
            renderer.texture,
            Rectangle{0, 0, static_cast<float>(renderer.texture.width), -static_cast<float>(renderer.texture.height)},
            Rectangle{0, 0, 512 * 4, 256 * 4},
            Vector2{0, 0},
            0.0f,
            WHITE);
    }
};
