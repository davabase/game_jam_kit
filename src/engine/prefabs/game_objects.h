#pragma once

#include "engine/framework.h"
#include "engine/prefabs/components.h"
#include "engine/prefabs/services.h"

class StaticBox : public GameObject
{
public:
    b2BodyId body = b2_nullBodyId;
    float x, y, width, height;

    StaticBox(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}

    void init() override
    {
        auto physics = scene->get_service<PhysicsService>();
        const float pixels_to_meters = physics->pixels_to_meters;
        auto world = physics->world;

        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = b2_staticBody;
        body_def.position = b2Vec2{x * pixels_to_meters, y * pixels_to_meters};
        body = b2CreateBody(world, &body_def);

        b2Polygon body_polygon = b2MakeBox(width / 2.0f * pixels_to_meters, height / 2.0f * pixels_to_meters);
        b2ShapeDef box_shape_def = b2DefaultShapeDef();
        b2CreatePolygonShape(body, &box_shape_def, &body_polygon);

        add_component<BodyComponent>(body);
    }

    void draw() override
    {
        DrawRectangle(x - width / 2.0f, y - height / 2.0f, width, height, RED);
    }
};

class DynamicBox : public GameObject
{
public:
    b2BodyId body = b2_nullBodyId;
    float x, y, width, height, rot_deg;
    PhysicsService* physics;

    DynamicBox(float x, float y, float width, float height, float rotation = 0) :
        x(x),
        y(y),
        width(width),
        height(height),
        rot_deg(rotation)
    {
    }

    DynamicBox(Vector2 position, Vector2 size, float rotation = 0) :
        x(position.x),
        y(position.y),
        width(size.x),
        height(size.y),
        rot_deg(rotation)
    {
    }

    void init() override
    {
        physics = scene->get_service<PhysicsService>();
        const float pixels_to_meters = physics->pixels_to_meters;
        auto world = physics->world;

        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = b2_dynamicBody;
        body_def.position = b2Vec2{x * pixels_to_meters, y * pixels_to_meters};
        body_def.rotation = b2MakeRot(rot_deg * DEG2RAD);
        body = b2CreateBody(world, &body_def);

        b2Polygon body_polygon = b2MakeBox(width / 2.0f * pixels_to_meters, height / 2.0f * pixels_to_meters);
        b2ShapeDef box_shape_def = b2DefaultShapeDef();
        b2SurfaceMaterial body_material = b2DefaultSurfaceMaterial();
        body_material.friction = 0.3f;
        box_shape_def.density = 1.0f;
        box_shape_def.material = body_material;
        b2CreatePolygonShape(body, &box_shape_def, &body_polygon);

        auto body_component = add_component<BodyComponent>(body);
        add_component<SpriteComponent>(body_component, "assets/character_green_idle.png");
    }

    void draw() override
    {
        float meters_to_pixels = physics->meters_to_pixels;
        b2Vec2 pos = b2Body_GetPosition(body);
        b2Rot rot = b2Body_GetRotation(body);
        float angle = b2Rot_GetAngle(rot) * RAD2DEG;

        DrawRectanglePro({physics->convert_to_pixels(pos.x), physics->convert_to_pixels(pos.y), width, height},
                         {width / 2.0f, height / 2.0f},
                         angle,
                         RED);
    }
};

class CameraObject : public GameObject
{
public:
    Camera2D camera;

    // The target position to follow, in pixels.
    Vector2 target = {0, 0};

    // The size of the screen.
    Vector2 size = {0, 0};

    // The size of the level in pixels. The camera will clamp to this size.
    Vector2 level_size = {0, 0};

    // Tracking speed in pixels per second.
    Vector2 follow_speed = {1000, 1000};

    // Deadzone bounds in pixels relative to the center.
    float offset_left = 150.0f;
    float offset_right = 150.0f;
    float offset_top = 100.0f;
    float offset_bottom = 100.0f;

    CameraObject(Vector2 size,
                 Vector2 level_size = {0, 0},
                 Vector2 follow_speed = {1000, 1000},
                 float offset_left = 150,
                 float offset_right = 150,
                 float offset_top = 100,
                 float offset_bottom = 100) :
        size(size),
        level_size(level_size),
        follow_speed(follow_speed),
        offset_left(offset_left),
        offset_right(offset_right),
        offset_top(offset_top),
        offset_bottom(offset_bottom)
    {
    }

    void init() override
    {
        camera.zoom = 1.0f;
        camera.offset = {size.x / 2.0f, size.y / 2.0f};
        camera.rotation = 0.0f;

        camera.target = target;
    }

    void update(float dt) override
    {
        // Desired camera.target after applying deadzone.
        Vector2 desired = camera.target;

        // Convert deadzone from SCREEN pixels to WORLD pixels (depends on zoom).
        // Because camera.target is in world units.
        float inv_zoom = (camera.zoom != 0.0f) ? (1.0f / camera.zoom) : 1.0f;

        float dz_left_w = offset_left * inv_zoom;
        float dz_right_w = offset_right * inv_zoom;
        float dz_top_w = offset_top * inv_zoom;
        float dz_bottom_w = offset_bottom * inv_zoom;

        // Compute target displacement from current camera center (world-space).
        float dx = target.x - camera.target.x;
        float dy = target.y - camera.target.y;

        // If target is outside deadzone, shift desired camera center just enough to bring it back.
        if (dx < -dz_left_w)
        {
            desired.x = target.x + dz_left_w;
        }
        else if (dx > dz_right_w)
        {
            desired.x = target.x - dz_right_w;
        }

        if (dy < -dz_top_w)
        {
            desired.y = target.y + dz_top_w;
        }
        else if (dy > dz_bottom_w)
        {
            desired.y = target.y - dz_bottom_w;
        }

        // Apply tracking speed per axis.
        if (follow_speed.x < 0)
        {
            camera.target.x = desired.x;
        }
        else
        {
            camera.target.x = move_towards(camera.target.x, desired.x, follow_speed.x * dt);
        }

        if (follow_speed.y < 0)
        {
            camera.target.y = desired.y;
        }
        else
        {
            camera.target.y = move_towards(camera.target.y, desired.y, follow_speed.y * dt);
        }

        Vector2 half_view = {size.x / 2.0f * inv_zoom, size.y / 2.0f * inv_zoom};
        if (level_size.x > size.x)
        {
            camera.target.x = clamp(camera.target.x, half_view.x, level_size.x - half_view.x);
        }
        if (level_size.y > size.y)
        {
            camera.target.y = clamp(camera.target.y, half_view.y, level_size.y - half_view.y);
        }
    }

    float move_towards(float current, float target, float maxDelta)
    {
        float d = target - current;
        if (d > maxDelta)
            return current + maxDelta;
        if (d < -maxDelta)
            return current - maxDelta;
        return target;
    }

    float clamp(float v, float lo, float hi)
    {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }

    void set_target(Vector2 target)
    {
        this->target = target;
    }

    void set_zoom(float zoom)
    {
        camera.zoom = zoom;
    }

    void set_rotation(float angle)
    {
        camera.rotation = angle;
    }

    void draw_begin()
    {
        BeginMode2D(camera);
    }

    void draw_end()
    {
        EndMode2D();
    }

    void debug_draw(Color c = {0, 255, 0, 120}) const
    {
        float inv_zoom = (camera.zoom != 0.0f) ? (1.0f / camera.zoom) : 1.0f;
        float dz_left_w = offset_left * inv_zoom;
        float dz_right_w = offset_right * inv_zoom;
        float dz_top_w = offset_top * inv_zoom;
        float dz_bottom_w = offset_bottom * inv_zoom;

        Rectangle r;
        r.x = camera.target.x - dz_left_w;
        r.y = camera.target.y - dz_top_w;
        r.width = dz_left_w + dz_right_w;
        r.height = dz_top_w + dz_bottom_w;

        DrawRectangleLinesEx(r, 2.0f * inv_zoom, c);
    }

    Vector2 screen_to_world(Vector2 point)
    {
        return GetScreenToWorld2D(point, camera);
    }
};

class SplitCamera : public CameraObject
{
public:
    RenderTexture2D texture;

    SplitCamera(Vector2 size,
                Vector2 level_size = {0, 0},
                Vector2 follow_speed = {1000, 1000},
                float offset_left = 150,
                float offset_right = 150,
                float offset_top = 100,
                float offset_bottom = 100) :
        CameraObject(size, level_size, follow_speed, offset_left, offset_right, offset_top, offset_bottom)
    {
    }

    ~SplitCamera()
    {
        UnloadRenderTexture(texture);
    }

    void init() override
    {
        texture = LoadRenderTexture(size.x, size.y);
        CameraObject::init();
    }

    void draw_begin()
    {
        BeginTextureMode(texture);
        ClearBackground(WHITE);
        BeginMode2D(camera);
    }

    void draw_end()
    {
        EndMode2D();
        EndTextureMode();
    }

    void draw_texture(float x, float y)
    {
        DrawTextureRec(texture.texture,
                       {0, 0, static_cast<float>(texture.texture.width), static_cast<float>(-texture.texture.height)},
                       {x, y},
                       WHITE);
    }

    Vector2 screen_to_world(Vector2 draw_position, Vector2 point)
    {
        auto local_point = point - draw_position;
        return GetScreenToWorld2D(local_point, camera);
    }
};

struct CharacterParams
{
    // Geometry in pixels
    float width = 24.0f;
    float height = 40.0f;

    // Initial position in pixels
    Vector2 position;

    // Surface behavior
    float friction = 0.0f;
    float restitution = 0.0f;
    float density = 1.0f;
};

class Character : public GameObject
{
public:
    CharacterParams p;
    PhysicsService* physics;
    BodyComponent* body;
    MovementComponent* movement;
    AnimationController* animation;
    TextComponent* text_component;
    SoundComponent* jump_sound;

    bool grounded = false;
    bool on_wall_left = false;
    bool on_wall_right = false;
    float coyote_timer = 0.0f;
    float jump_buffer_timer = 0.0f;

    Character(CharacterParams p) : p(p) {}

    void init() override
    {
        physics = scene->get_service<PhysicsService>();

        body = add_component<BodyComponent>(
            [=](BodyComponent& b)
            {
                b2BodyDef body_def = b2DefaultBodyDef();
                body_def.type = b2_dynamicBody;
                body_def.fixedRotation = true;
                body_def.isBullet = true;
                body_def.linearDamping = 0.0f;
                body_def.angularDamping = 0.0f;
                body_def.position = physics->convert_to_meters(p.position);
                b.id = b2CreateBody(physics->world, &body_def);

                b2SurfaceMaterial body_material = b2DefaultSurfaceMaterial();
                body_material.friction = p.friction;
                body_material.restitution = p.restitution;

                b2ShapeDef box_shape_def = b2DefaultShapeDef();
                box_shape_def.density = p.density;
                box_shape_def.material = body_material;

                b2Polygon body_polygon = b2MakeRoundedBox(physics->convert_to_meters(p.width / 2.0f),
                                                          physics->convert_to_meters(p.height / 2.0f),
                                                          physics->convert_to_meters(0.25));
                b2CreatePolygonShape(b.id, &box_shape_def, &body_polygon);
            });

        MovementParams mp;
        mp.width = p.width;
        mp.height = p.height;
        movement = add_component<MovementComponent>(mp);

        animation = add_component<AnimationController>(body);
        animation->add_animation("idle", std::vector<std::string>{"assets/character_green_idle.png"}, 1.0f);
        animation->add_animation(
            "walk",
            std::vector<std::string>{"assets/character_green_walk_a.png", "assets/character_green_walk_b.png"},
            5.0f);
        animation->play("walk");
        animation->set_scale(0.35f);

        text_component = add_component<TextComponent>("Character", "Roboto", 128, WHITE);
        text_component->position = {p.position.x, p.position.y - p.height / 2.0f - 20.0f};

        jump_sound = add_component<SoundComponent>("assets/explosionCrunch_000.ogg");
    }

    void update(float delta_time) override
    {
        int gamepad = 0;
        float deadzone = 0.1f;

        const bool jump_pressed =
            IsKeyPressed(KEY_W) || IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        const bool jump_held = IsKeyDown(KEY_W) || IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

        float move_x = 0.0f;
        move_x = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
        if (fabsf(move_x) < deadzone)
        {
            move_x = 0.0f;
        }
        if (IsKeyDown(KEY_D) || IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
        {
            move_x = 1.0f;
        }
        else if (IsKeyDown(KEY_A) || IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
        {
            move_x = -1.0f;
        }

        if (move_x < 0.0f)
        {
            animation->set_flip_x(true);
        }
        else if (move_x > 0.0f)
        {
            animation->set_flip_x(false);
        }
        if (fabsf(move_x) > 0.1f)
        {
            animation->play("walk");
        }
        else
        {
            animation->play("idle");
        }

        if (jump_pressed)
        {
            jump_sound->play();
        }

        movement->set_input(move_x, jump_pressed, jump_held);
    }

    void draw() override
    {
        Color color = movement->grounded ? GREEN : BLUE;
        auto pos = body->get_position_pixels();
        DrawRectanglePro({pos.x, pos.y, p.width, p.height}, {p.width / 2.0f, p.height / 2.0f}, 0.0f, color);
    }
};
