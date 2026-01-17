#pragma once

#include "engine/physics_debug.h"
#include "engine/prefabs/includes.h"

class Playground : public Scene
{
public:
    void init_services() override
    {
        add_service<TextureService>();
        add_service<SoundService>();
        add_service<PhysicsService>();
        std::vector<std::string> collision_names = {"walls"};
        add_service<LevelService>("assets/AutoLayers_1_basic.ldtk", "AutoLayer", collision_names);
    }

    void init() override
    {
        auto level = get_service<LevelService>();
        auto player_entity = level->get_entity_by_name("Player");
        if (!player_entity)
        {
            assert(false);
        }
        auto box_entity = level->get_entity_by_tag("box");
        if (!box_entity)
        {
            assert(false);
        }

        CharacterParams params;
        params.position = level->convert_to_pixels(player_entity->getPosition());
        auto character = add_game_object<PlatformerCharacter>(params);
        character->add_tag("character");

        auto position = level->convert_to_pixels(box_entity->getPosition());
        auto size = level->convert_to_pixels(box_entity->getSize());
        auto box = add_game_object<DynamicBox>(position, size, 46.0f);

        auto ground = add_game_object<StaticBox>(400.0f, 587.5f, 800.0f, 25.0f);
        ground->add_tag("ground");

        // auto camera = add_game_object<CameraObject>(Vector2{800, 600}, level->get_size());
        // camera->add_tag("camera");

        auto split_camera = add_game_object<SplitCamera>(Vector2{400, 600}, level->get_size());
        split_camera->add_tag("camera");

        auto split_camera2 = add_game_object<SplitCamera>(Vector2{400, 600}, level->get_size());
        split_camera2->add_tag("camera");
    }

    void update(float delta_time) override
    {
        auto camera = static_cast<SplitCamera*>(get_game_objects_with_tag("camera")[0]);
        auto camera2 = static_cast<SplitCamera*>(get_game_objects_with_tag("camera")[1]);
        auto player = static_cast<PlatformerCharacter*>(get_game_objects_with_tag("character")[0]);
        auto physics = get_service<PhysicsService>();
        camera->target = player->body->get_position_pixels();
        camera2->target = player->body->get_position_pixels();

        auto local = camera->screen_to_world({0, 0}, GetMousePosition());
        Rectangle rec = {local.x - 10, local.y - 10, 20, 20};
        auto contacts = physics->rectangle_overlap(rec, 0);

        if (IsKeyPressed(KEY_SPACE))
        {
            game->go_to_scene_next();
        }
    }

    void draw_scene() override
    {
        auto camera = static_cast<SplitCamera*>(get_game_objects_with_tag("camera")[0]);
        auto camera2 = static_cast<SplitCamera*>(get_game_objects_with_tag("camera")[1]);
        auto physics = get_service<PhysicsService>();

        // The scene needs to be rendered for each camera.
        camera->draw_begin();
        Scene::draw_scene();
        physics->draw_debug();
        auto local = camera->screen_to_world({0, 0}, GetMousePosition());
        Rectangle rec = {local.x - 10, local.y - 10, 20, 20};
        DrawRectangle(rec.x, rec.y, rec.width, rec.height, MAGENTA);
        camera->draw_end();

        camera2->draw_begin();
        Scene::draw_scene();
        camera2->draw_end();

        // Draw the resulting views.
        camera->draw_texture(0, 0);
        camera2->draw_texture(400, 0);

        DrawLine(400, 0, 400, 600, GRAY);
    }
};
