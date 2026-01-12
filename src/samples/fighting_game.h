#pragma once

#include "engine/prefabs/includes.h"

class Fighting : public Scene
{
public:
    void init_services() override
    {
        add_service<TextureService>();
        add_service<SoundService>();
        add_service<PhysicsService>();
        std::vector<std::string> collision_names = {"walls"};
        add_service<LevelService>("assets/levels/fighting.ldtk", "Stage", collision_names);
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
        auto character = add_game_object<Character>(params);
        character->add_tag("character");

        auto camera = add_game_object<CameraObject>(Vector2{800, 600}, level->get_size());
        camera->add_tag("camera");
    }

    void update(float delta_time) override
    {
        auto camera = static_cast<CameraObject*>(get_game_objects_with_tag("camera")[0]);
        auto player = static_cast<Character*>(get_game_objects_with_tag("character")[0]);
        auto physics = get_service<PhysicsService>();
        // camera->target = player->body->get_position_pixels();
    }

    void draw_scene() override
    {
        auto camera = static_cast<CameraObject*>(get_game_objects_with_tag("camera")[0]);
        auto physics = get_service<PhysicsService>();

        // The scene needs to be rendered for each camera.
        camera->draw_begin();
        Scene::draw_scene();
        camera->draw_end();
    }
};
