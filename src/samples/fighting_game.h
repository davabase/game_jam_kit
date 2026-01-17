/**
 * Demonstration of a shared camera, multiple characters, and basic fighting mechanics.
 * Shows how to setup a level with the LevelService and physics bodies with the PhysicsService.
 * Also shows how to use animations and sounds.
 */

#pragma once

#include "engine/prefabs/includes.h"

/**
 * A basic fighting character.
 * See also engine/prefabs/game_objects.h for PlatformerCharacter.
 */
class FightingCharacter : public GameObject
{
public:
    CharacterParams p;
    PhysicsService* physics;
    LevelService* level;
    BodyComponent* body;
    MovementComponent* movement;
    AnimationController* animation;
    MultiComponent<SoundComponent>* sounds;
    SoundComponent* jump_sound;
    SoundComponent* hit_sound;
    SoundComponent* die_sound;

    bool grounded = false;
    bool on_wall_left = false;
    bool on_wall_right = false;
    float coyote_timer = 0.0f;
    float jump_buffer_timer = 0.0f;
    int gamepad = 0;
    int player_number = 1;
    float width = 24.0f;
    float height = 40.0f;
    bool fall_through = false;
    float fall_through_timer = 0.0f;
    float fall_through_duration = 0.2f;
    float attack_display_timer = 0.0f;
    float attack_display_duration = 0.1f;
    bool attack = false;

    FightingCharacter(CharacterParams p, int player_number = 1) :
        p(p),
        player_number(player_number),
        gamepad(player_number - 1),
        width(p.width),
        height(p.height)
    {
    }

    void init() override
    {

        // Grab the physics service.
        // All get_service calls should be done in init(). get_service is not quick and this also allows us to test
        // that all services exist during init time.
        physics = scene->get_service<PhysicsService>();

        // Setup the character's physics body using the BodyComponent initialization callback lambda.
        body = add_component<BodyComponent>(
            [=](BodyComponent& b)
            {
                b2BodyDef body_def = b2DefaultBodyDef();
                body_def.type = b2_dynamicBody;
                body_def.fixedRotation = true;
                body_def.isBullet = true;
                // All units in box2d are in meters.
                body_def.position = physics->convert_to_meters(p.position);
                // Assign this GameObject as the user data so we can find it in collision callbacks.
                body_def.userData = this;
                b.id = b2CreateBody(physics->world, &body_def);

                b2SurfaceMaterial body_material = b2DefaultSurfaceMaterial();
                body_material.friction = p.friction;
                body_material.restitution = p.restitution;

                b2ShapeDef box_shape_def = b2DefaultShapeDef();
                box_shape_def.density = p.density;
                box_shape_def.material = body_material;

                // Needed to presolve one-way behavior.
                box_shape_def.enablePreSolveEvents = true;

                // We use a rounded box which helps with getting stuck on edges.
                b2Polygon body_polygon = b2MakeRoundedBox(physics->convert_to_meters(p.width / 2.0f),
                                                          physics->convert_to_meters(p.height / 2.0f),
                                                          physics->convert_to_meters(0.25));
                b2CreatePolygonShape(b.id, &box_shape_def, &body_polygon);
            });

        MovementParams mp;
        mp.width = p.width;
        mp.height = p.height;
        movement = add_component<MovementComponent>(mp);

        level = scene->get_service<LevelService>();

        // TODO: Is only allowing one component per type really as cool an idea as I thought?
        sounds = add_component<MultiComponent<SoundComponent>>();
        jump_sound = sounds->add_component("jump", "assets/sounds/jump.wav");
        hit_sound = sounds->add_component("hit", "assets/sounds/hit.wav");
        die_sound = sounds->add_component("die", "assets/sounds/die.wav");

        // Setup animations.
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
        // Get input and route to movement component.
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

        movement->set_input(move_x, jump_pressed, jump_held);

        if (movement->grounded && jump_pressed)
        {
            jump_sound->play();
        }

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

        // Custom one-way platform fall-through logic.
        float move_y = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);
        if (IsKeyPressed(KEY_S) || IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN) || move_y > 0.5f)
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

        // Attack logic.
        if (IsKeyPressed(KEY_SPACE) || IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
        {
            attack = true;
            attack_display_timer = attack_display_duration;
            Vector2 position = body->get_position_pixels();
            position.x += (width / 2.0f + 8.0f) * (animation->flip_x ? -1.0f : 1.0f);
            auto bodies = physics->circle_overlap(position, 8.0f, body->id);
            for (auto& other_body : bodies)
            {
                // Apply impulse to other character.
                b2Vec2 impulse = b2Vec2{animation->flip_x ? -10.0f : 10.0f, -10.0f};
                b2Body_ApplyLinearImpulse(other_body, impulse, b2Body_GetPosition(other_body), true);
                hit_sound->play();
            }
        }

        if (attack_display_timer > 0.0f)
        {
            attack_display_timer = std::max(0.0f, attack_display_timer - delta_time);
            if (attack_display_timer == 0.0f)
            {
                attack = false;
            }
        }

        // Death and respawn logic.
        if (body->get_position_pixels().y > level->get_size().y + 200.0f)
        {
            // Re-spawn at start position.
            body->set_position(p.position);
            body->set_velocity(Vector2{0.0f, 0.0f});
            die_sound->play();
        }
    }

    void draw() override
    {
        // Draw attack indicator.
        if (attack)
        {
            Vector2 position = body->get_position_pixels();
            position.x += (width / 2.0f + 8.0f) * (animation->flip_x ? -1.0f : 1.0f);
            DrawCircleV(position, 8.0f, Fade(RED, 0.5f));
        }
        // Animations are drawn by the AnimationController component.
    }

    /**
     * Pre-solve callback for one-way platforms.
     *
     * @param body_a The first body in the collision.
     * @param body_b The second body in the collision.
     * @param manifold The contact manifold.
     * @param platforms The list of one-way platform StaticBox objects.
     * @return true to enable the contact, false to disable it.
     */
    bool PreSolve(b2BodyId body_a,
                  b2BodyId body_b,
                  b2Manifold* manifold,
                  std::vector<std::shared_ptr<StaticBox>> platforms) const
    {
        float sign = 0.0f;
        b2BodyId other = b2_nullBodyId;

        // Check which body is the character.
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
        // TextureService and SoundService are needed by other components and game objects.
        add_service<TextureService>();
        add_service<SoundService>();

        // PhysicsService is used by LevelService and must be added first.
        physics = add_service<PhysicsService>();
        // Setup LDtk level. Checkout the file in LDtk editor to see how it's built.
        std::vector<std::string> collision_names = {"walls"};
        level = add_service<LevelService>("assets/levels/fighting.ldtk", "Stage", collision_names);
    }

    void init() override
    {
        auto window_manager = game->get_manager<WindowManager>();

        // Find one-way platform entities in the level and create StaticBox game objects for them.
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

        // Pre-solve callback to handle one-way platforms.
        b2World_SetPreSolveCallback(physics->world, PreSolveStatic, this);

        // Create player characters at the "Start" entities.
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

        // Setup shared camera.
        camera = add_game_object<CameraObject>(level->get_size(), Vector2{0, 0}, Vector2{300, 300}, 0, 0, 0, 0);
        camera->target = level->get_size() / 2.0f;

        // Disable the background layer from drawing. We'll draw it manually in draw_scene().
        level->set_layer_visibility("Background", false);

        renderer = LoadRenderTexture(level->get_size().x, level->get_size().y);
    }

    void update(float delta_time) override
    {
        // Set the camera target to the center of all players.
        // Also zoom to fit all players.
        Vector2 avg_position = {0.0f, 0.0f};
        Vector2 min_point = {FLT_MAX, FLT_MAX};
        Vector2 max_point = {-FLT_MAX, -FLT_MAX};
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

        // Force camera to align with pixel grid to avoid sub-pixel jitter.
        camera->target.x = floorf(camera->target.x);
        camera->target.y = floorf(camera->target.y);

        // Calculate zoom to fit all characters.
        float distance = sqrtf((max_point.x - min_point.x) * (max_point.x - min_point.x) +
                               (max_point.y - min_point.y) * (max_point.y - min_point.y));
        float level_diagonal =
            sqrtf(level->get_size().x * level->get_size().x + level->get_size().y * level->get_size().y);
        float zoom = level_diagonal / (distance + 400);
        zoom = std::clamp(zoom, 0.5f, 2.0f);

        // Lerp zoom for smoothness.
        // Note that this style of camera zoom is incompatible with pixel perfect rendering. Remove this line to see
        // pixel perfect scaling.
        camera->camera.zoom += (zoom - camera->camera.zoom) * std::min(1.0f, delta_time * 5.0f);

        // Draw the level centered in the window.
        float aspect_ratio = level->get_size().x / level->get_size().y;
        float render_scale = GetScreenHeight() / level->get_size().y;
        Vector2 render_size = {level->get_size().y * render_scale * aspect_ratio, level->get_size().y * render_scale};
        auto pos = (Vector2{(float)GetScreenWidth(), (float)GetScreenHeight()} - render_size) / 2.0f;
        render_rect = Rectangle{pos.x, pos.y, render_size.x, render_size.y};
    }

    /**
     * We override draw_scene instead of draw to control when Scene::draw_scene is called.
     * This allows us to draw the scene inside the camera's Begin/End block and render the camera to a texture.
     */
    void draw_scene() override
    {
        // Draw to render texture.
        BeginTextureMode(renderer);
        ClearBackground(MAGENTA);

        // Draw the background layer outside of the camera.
        level->draw_layer("Background");

        // Start the camera and render the scene inside.
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

    /**
     * Static pre-solve callback to route to the appropriate character.
     *
     * @param shape_a The first shape in the collision.
     * @param shape_b The second shape in the collision.
     * @param manifold The contact manifold.
     * @param context The FightingScene instance.
     * @return true to enable the contact, false to disable it.
     */
    static bool PreSolveStatic(b2ShapeId shape_a, b2ShapeId shape_b, b2Manifold* manifold, void* context)
    {
        FightingScene* self = static_cast<FightingScene*>(context);
        b2BodyId body_a = b2Shape_GetBody(shape_a);
        b2BodyId body_b = b2Shape_GetBody(shape_b);

        // Find which character is involved and route to its PreSolve method.
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
