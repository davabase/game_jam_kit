#pragma once

#include "engine/prefabs/includes.h"

class FightingCharacter : public Character
{
public:
    AnimationController* animation;
    int player_number = 1;
    float width = 24.0f;
    float height = 40.0f;
    bool fall_through = false;
    float fall_through_timer = 0.0f;
    float fall_through_duration = 0.2f;
    LevelService* level = nullptr;

    FightingCharacter(CharacterParams p, int player_number = 1) :
        Character(p, player_number - 1),
        player_number(player_number),
        width(p.width),
        height(p.height)
    {
    }

    void init() override
    {
        Character::init();

        level = scene->get_service<LevelService>();

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
            animation->add_animation("jump", std::vector<std::string>{"assets/sunnyland/fox/jump-1.png"}, 0.0f);
            animation->add_animation("fall", std::vector<std::string>{"assets/sunnyland/fox/jump-2.png"}, 0.0f);

            animation->origin.y += 4;
        }
        else if (player_number == 2)
        {
            animation->add_animation("run",
                                     std::vector<std::string>{"assets/sunnyland/bunny/run-1.png",
                                                              "assets/sunnyland/bunny/run-2.png",
                                                              "assets/sunnyland/bunny/run-3.png",
                                                              "assets/sunnyland/bunny/run-4.png",
                                                              "assets/sunnyland/bunny/run-5.png",
                                                              "assets/sunnyland/bunny/run-6.png"},
                                     10.0f);
            animation->add_animation("idle",
                                     std::vector<std::string>{"assets/sunnyland/bunny/idle-1.png",
                                                              "assets/sunnyland/bunny/idle-2.png",
                                                              "assets/sunnyland/bunny/idle-3.png",
                                                              "assets/sunnyland/bunny/idle-4.png"},
                                     10.0f);
            animation->add_animation("jump", std::vector<std::string>{"assets/sunnyland/bunny/jump-1.png"}, 0.0f);
            animation->add_animation("fall", std::vector<std::string>{"assets/sunnyland/bunny/jump-2.png"}, 0.0f);

            animation->origin.y += 8;
        }
        else if (player_number == 3)
        {
            animation->add_animation("run",
                                     std::vector<std::string>{"assets/sunnyland/squirrel/run-1.png",
                                                              "assets/sunnyland/squirrel/run-2.png",
                                                              "assets/sunnyland/squirrel/run-3.png",
                                                              "assets/sunnyland/squirrel/run-4.png",
                                                              "assets/sunnyland/squirrel/run-5.png",
                                                              "assets/sunnyland/squirrel/run-6.png"},
                                     10.0f);
            animation->add_animation("idle",
                                     std::vector<std::string>{"assets/sunnyland/squirrel/idle-1.png",
                                                              "assets/sunnyland/squirrel/idle-2.png",
                                                              "assets/sunnyland/squirrel/idle-3.png",
                                                              "assets/sunnyland/squirrel/idle-4.png",
                                                              "assets/sunnyland/squirrel/idle-5.png",
                                                              "assets/sunnyland/squirrel/idle-6.png",
                                                              "assets/sunnyland/squirrel/idle-7.png",
                                                              "assets/sunnyland/squirrel/idle-8.png"},
                                     8.0f);
            animation->add_animation("jump",
                                     std::vector<std::string>{"assets/sunnyland/squirrel/jump-1.png",
                                                              "assets/sunnyland/squirrel/jump-2.png",
                                                              "assets/sunnyland/squirrel/jump-3.png",
                                                              "assets/sunnyland/squirrel/jump-4.png"},
                                     15.0f);
            animation->origin.y += 7;
        }
        else if (player_number == 4)
        {
            animation->add_animation("run",
                                     std::vector<std::string>{"assets/sunnyland/imp/run-1.png",
                                                              "assets/sunnyland/imp/run-2.png",
                                                              "assets/sunnyland/imp/run-3.png",
                                                              "assets/sunnyland/imp/run-4.png",
                                                              "assets/sunnyland/imp/run-5.png",
                                                              "assets/sunnyland/imp/run-6.png",
                                                              "assets/sunnyland/imp/run-7.png",
                                                              "assets/sunnyland/imp/run-8.png"},
                                     10.0f);
            animation->add_animation("idle",
                                     std::vector<std::string>{"assets/sunnyland/imp/idle-1.png",
                                                              "assets/sunnyland/imp/idle-2.png",
                                                              "assets/sunnyland/imp/idle-3.png",
                                                              "assets/sunnyland/imp/idle-4.png"},
                                     10.0f);
            animation->add_animation("jump", std::vector<std::string>{"assets/sunnyland/imp/jump-1.png"}, 0.0f);
            animation->add_animation("fall", std::vector<std::string>{"assets/sunnyland/imp/jump-4.png"}, 0.0f);
            animation->origin.y += 10;
        }
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

        if (!movement->grounded)
        {
            // Squirrel doesn't have a fall animation so we do this monstrosity.
            if (player_number != 3)
            {
                if (body->get_velocity_meters().y < 0.0f)
                {
                    animation->play("jump");
                }
                else
                {

                    animation->play("fall");
                }
            }
            else
            {
                animation->play("jump");
            }
        }

        if (IsKeyPressed(KEY_S) || IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN))
        {
            fall_through = true;
            fall_through_timer = fall_through_duration;
        }

        if (fall_through_timer > 0.0f)
        {
            fall_through_timer = std::max(0.0f, fall_through_timer - delta_time);
            if (fall_through_timer == 0.0f)
            {
                fall_through = false;
            }
        }

        if (IsKeyPressed(KEY_SPACE) || IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
        {
            Vector2 position = body->get_position_pixels();
            position.x += (width / 2.0f + 8.0f) * (animation->flip_x ? -1.0f : 1.0f);
            auto bodies = physics->circle_overlap(position, 8.0f, body->id);
            for (auto& other_body : bodies)
            {
                // Apply impulse to other character.
                b2Vec2 impulse = b2Vec2{animation->flip_x ? -10.0f : 10.0f, -10.0f};
                b2Body_ApplyLinearImpulse(other_body, impulse, b2Body_GetPosition(other_body), true);
            }
        }

        if (body->get_position_pixels().y > level->get_size().y + 200.0f)
        {
            // Re-spawn at start position.
            body->set_position(p.position);
            body->set_velocity(Vector2{0.0f, 0.0f});
        }
    }

    void draw() override
    {
        // Vector2 position = body->get_position_pixels();
        // position.x += (width / 2.0f + 8.0f) * (animation->flip_x ? -1.0f : 1.0f);
        // DrawCircleV(position, 8.0f, Fade(RED, 0.5f));
        // Character is drawn by the AnimationController.
    }

    bool PreSolve(b2BodyId body_a,
                  b2BodyId body_b,
                  b2Manifold* manifold,
                  std::vector<std::shared_ptr<StaticBox>> platforms) const
    {
        float sign = 0.0f;
        b2BodyId other = b2_nullBodyId;
        if (body_a == body->id)
        {
            sign = 1.0f;
            other = body_b;
        }
        else if (body_b == body->id)
        {
            sign = -1.0f;
            other = body_a;
        }

        if (sign * manifold->normal.y < 0.5f)
        {
            // Normal points down, disable contact.
            return false;
        }

        if (fall_through)
        {
            for (auto& platform : platforms)
            {
                if (other == platform->body)
                {
                    // Character is in fall-through state, disable contact.
                    return false;
                }
            }
        }

        // Otherwise, enable contact.
        return true;
    }
};

/**
 * A fighting game scene.
 */
class FightingScene : public Scene
{
public:
    RenderTexture2D renderer;
    Rectangle render_rect;
    std::vector<std::shared_ptr<StaticBox>> platforms;
    std::vector<std::shared_ptr<FightingCharacter>> characters;
    LevelService* level;
    PhysicsService* physics;
    std::shared_ptr<CameraObject> camera;

    void init_services() override
    {
        add_service<TextureService>();
        add_service<SoundService>();
        add_service<PhysicsService>();
        std::vector<std::string> collision_names = {"walls"};
        level = add_service<LevelService>("assets/levels/fighting.ldtk", "Stage", collision_names);
    }

    void init() override
    {
        auto window_manager = get_manager<WindowManager>();

        auto platform_entities = level->get_entities_by_name("One_way_platform");
        for (auto& platform_entity : platform_entities)
        {
            Vector2 position = level->convert_to_pixels(platform_entity->getPosition());
            Vector2 size = level->convert_to_pixels(platform_entity->getSize());
            auto platform = add_game_object<StaticBox>(position + size / 2.0f, size);
            platform->is_visible = false;
            platform->add_tag("platform");
            platforms.push_back(platform);
        }

        physics = get_service<PhysicsService>();

        // Pre-solve callback to handle one-way platforms.
        b2World_SetPreSolveCallback(physics->world, PreSolveStatic, this);

        auto player_entities = level->get_entities_by_name("Start");

        for (int i = 0; i < player_entities.size() && i < 4; i++)
        {
            auto& player_entity = player_entities[i];
            CharacterParams params;
            params.position = level->convert_to_pixels(player_entity->getPosition());
            params.width = 16;
            params.height = 24;
            auto character = add_game_object<FightingCharacter>(params, i + 1);
            character->add_tag("character");
            characters.push_back(character);
        }

        camera = add_game_object<CameraObject>(level->get_size(), Vector2{0, 0}, Vector2{300, 300}, 0, 0, 0, 0);
        camera->target = level->get_size() / 2.0f;

        // Disable the background layer from drawing.
        level->set_layer_visibility("Background", false);

        renderer = LoadRenderTexture(level->get_size().x, level->get_size().y);
        float aspect_ratio = level->get_size().x / level->get_size().y;
        float render_scale = window_manager->get_height() / level->get_size().y;
        Vector2 render_size = {level->get_size().y * render_scale * aspect_ratio, level->get_size().y * render_scale};
        auto pos = (window_manager->get_size() - render_size) / 2.0f;
        render_rect = Rectangle{pos.x, pos.y, render_size.x, render_size.y};
    }

    void update(float delta_time) override
    {
        // Set the camera target to the center of all players.
        // Also zoom to fit all players.
        Vector2 avg_position = {0.0f, 0.0f};
        Vector2 min_point = {FLT_MAX, FLT_MAX};
        Vector2 max_point = {FLT_MIN, FLT_MIN};
        for (auto& character : characters)
        {
            Vector2 char_pos = character->body->get_position_pixels();
            avg_position = avg_position + char_pos;
            min_point.x = std::min(min_point.x, char_pos.x);
            min_point.y = std::min(min_point.y, char_pos.y);
            max_point.x = std::max(max_point.x, char_pos.x);
            max_point.y = std::max(max_point.y, char_pos.y);
        }
        avg_position = avg_position / static_cast<float>(characters.size());
        camera->target = avg_position;

        float distance = sqrtf((max_point.x - min_point.x) * (max_point.x - min_point.x) +
                               (max_point.y - min_point.y) * (max_point.y - min_point.y));
        float level_diagonal =
            sqrtf(level->get_size().x * level->get_size().x + level->get_size().y * level->get_size().y);
        float zoom = level_diagonal / (distance + 400);
        zoom = std::clamp(zoom, 0.5f, 2.0f);
        // Lerp zoom for smoothness.
        camera->camera.zoom += (zoom - camera->camera.zoom) * std::min(1.0f, delta_time * 5.0f);
    }

    void draw_scene() override
    {
        BeginTextureMode(renderer);
        ClearBackground(MAGENTA);

        // Draw the background layer outside of the camera.
        level->draw_layer("Background");

        camera->draw_begin();
        Scene::draw_scene();
        // physics->draw_debug();
        // camera->draw_debug();
        camera->draw_end();

        EndTextureMode();

        // Draw centered.
        DrawTexturePro(
            renderer.texture,
            Rectangle{0, 0, static_cast<float>(renderer.texture.width), -static_cast<float>(renderer.texture.height)},
            render_rect,
            Vector2{0.0f, 0.0f},
            0.0f,
            WHITE);
    }

    static bool PreSolveStatic(b2ShapeId shape_a, b2ShapeId shape_b, b2Manifold* manifold, void* context)
    {
        FightingScene* self = static_cast<FightingScene*>(context);
        b2BodyId body_a = b2Shape_GetBody(shape_a);
        b2BodyId body_b = b2Shape_GetBody(shape_b);
        for (auto& character : self->characters)
        {
            if (body_a == character->body->id || body_b == character->body->id)
            {
                return character->PreSolve(body_a, body_b, manifold, self->platforms);
            }
        }
        return true;
    }
};
