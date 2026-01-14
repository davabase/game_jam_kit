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
            animation->add_animation("jump", std::vector<std::string>{"assets/sunnyland/fox/jump-1.png"}, 0.0f);
            animation->add_animation("fall", std::vector<std::string>{"assets/sunnyland/fox/jump-2.png"}, 0.0f);

            animation->origin.y += 4;
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
            if (body->get_velocity_meters().y < 0.0f)
            {
                animation->play("jump");
            }
            else
            {
                animation->play("fall");
            }
        }
    }

    void draw() override
    {
        // Character is drawn by the AnimationController.
    }
};

class OneWayPlatform : public GameObject
{
public:
    BodyComponent* body;
    Vector2 position;
    Vector2 size;

    OneWayPlatform(Vector2 position, Vector2 size) : position(position), size(size) {}

    void init() override
    {
        auto physics = scene->get_service<PhysicsService>();

        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = b2_staticBody;
        body_def.position = physics->convert_to_meters(position);
        auto body_id = b2CreateBody(physics->world, &body_def);

        b2Polygon body_polygon =
            b2MakeBox(physics->convert_to_meters(size.x / 2.0f), physics->convert_to_meters(size.y / 2.0f));
        b2ShapeDef box_shape_def = b2DefaultShapeDef();

        // Needed to presolve one-way behavior.
        box_shape_def.enablePreSolveEvents = true;

        b2CreatePolygonShape(body_id, &box_shape_def, &body_polygon);

        body = add_component<BodyComponent>(body_id);
    }

    void draw() override
    {
        // Draw nothing.
    }

    bool PreSolve(b2BodyId body_a, b2BodyId body_b, b2Manifold* manifold) const
    {
        float sign = 0.0f;
        if (body_a == body->id)
        {
            sign = -1.0f;
        }
        else if (body_b == body->id)
        {
            sign = 1.0f;
        }

        if (sign * manifold->normal.y < 0.5f)
        {
            // Normal points down, disable contact.
            return false;
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
    std::vector<std::shared_ptr<OneWayPlatform>> platforms;
    LevelService* level;

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

        auto player_entity = level->get_entities_by_name("Start")[2];
        if (!player_entity)
        {
            assert(false);
        }

        auto platform_entities = level->get_entities_by_name("One_way_platform");
        for (auto& platform_entity : platform_entities)
        {
            Vector2 position = level->convert_to_pixels(platform_entity->getPosition());
            Vector2 size = level->convert_to_pixels(platform_entity->getSize());
            auto platform = add_game_object<OneWayPlatform>(position + size / 2.0f, size);
            platform->add_tag("platform");
            platforms.push_back(platform);
        }

        b2World_SetPreSolveCallback(get_service<PhysicsService>()->world, PreSolveStatic, this);

        CharacterParams params;
        params.position = level->convert_to_pixels(player_entity->getPosition());
        params.width = 16;
        params.height = 24;
        auto character = add_game_object<FightingCharacter>(params);
        character->add_tag("character");

        auto camera = add_game_object<CameraObject>(level->get_size());
        camera->add_tag("camera");

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

        // Draw the background layer outside of the camera.
        level->draw_layer("Background");

        camera->draw_begin();
        Scene::draw_scene();
        physics->draw_debug();
        camera->draw_debug();
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
        for (auto& platform : self->platforms)
        {
            if (body_a == platform->body->id || body_b == platform->body->id)
            {
                return platform->PreSolve(body_a, body_b, manifold);
            }
        }
        return true;
    }
};
