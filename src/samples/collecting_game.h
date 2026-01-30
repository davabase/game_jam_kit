/**
 * Demonstration of split screen camera system and sensors for collecting items.
 */

#pragma once

#include "engine/prefabs/includes.h"

/**
 * A basic collecting character.
 * See also engine/prefabs/game_objects.h for PlatformerCharacter.
 */
class CollectingCharacter : public GameObject
{
public:
    CharacterParams p;
    PhysicsService* physics;
    LevelService* level;
    BodyComponent* body;
    PlatformerMovementComponent* movement;
    AnimationController* animation;
    MultiComponent<SoundComponent>* sounds;
    SoundComponent* jump_sound;
    SoundComponent* die_sound;
    int score = 0;

    bool grounded = false;
    bool on_wall_left = false;
    bool on_wall_right = false;
    float coyote_timer = 0.0f;
    float jump_buffer_timer = 0.0f;
    int gamepad = 0;
    int player_number = 1;
    float width = 24.0f;
    float height = 24.0f;

    CollectingCharacter(CharacterParams p, int player_number = 1) :
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

                // Needed to get sensor events.
                box_shape_def.enableSensorEvents = true;

                // We use a rounded box which helps with getting stuck on edges.
                b2Polygon body_polygon = b2MakeRoundedBox(physics->convert_to_meters(p.width / 2.0f),
                                                          physics->convert_to_meters(p.height / 2.0f),
                                                          physics->convert_to_meters(0.25));
                b2CreatePolygonShape(b.id, &box_shape_def, &body_polygon);
            });

        PlatformerMovementParams mp;
        mp.width = p.width;
        mp.height = p.height;
        movement = add_component<PlatformerMovementComponent>(mp);

        level = scene->get_service<LevelService>();

        // TODO: Is only allowing one component per type really as cool an idea as I thought?
        sounds = add_component<MultiComponent<SoundComponent>>();
        jump_sound = sounds->add_component("jump", "assets/sounds/jump.wav");
        die_sound = sounds->add_component("die", "assets/sounds/die.wav");

        // Setup animations.
        animation = add_component<AnimationController>(body);
        if (player_number == 1)
        {
            animation->add_animation("run",
                                     std::vector<std::string>{"assets/pixel_platformer/characters/green_1.png",
                                                              "assets/pixel_platformer/characters/green_2.png"},
                                     10.0f);
        }
        else if (player_number == 2)
        {
            animation->add_animation("run",
                                     std::vector<std::string>{"assets/pixel_platformer/characters/blue_1.png",
                                                              "assets/pixel_platformer/characters/blue_2.png"},
                                     10.0f);
        }
        else if (player_number == 3)
        {
            animation->add_animation("run",
                                     std::vector<std::string>{"assets/pixel_platformer/characters/pink_1.png",
                                                              "assets/pixel_platformer/characters/pink_2.png"},
                                     10.0f);
        }
        else if (player_number == 4)
        {
            animation->add_animation("run",
                                     std::vector<std::string>{"assets/pixel_platformer/characters/yellow_1.png",
                                                              "assets/pixel_platformer/characters/yellow_2.png"},
                                     10.0f);
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
            animation->flip_x = movement->move_x > 0.0f;
        }
        else
        {
            animation->pause();
        }
    }

    void die()
    {
        // Re-spawn at start position.
        body->set_position(p.position);
        body->set_velocity(Vector2{0.0f, 0.0f});
        die_sound->play();
    }
};

enum EnemyType
{
    Bat,
    DrillHead,
    BlockHead
};

/**
 * An enemy.
 */
class Enemy : public GameObject
{
public:
    Vector2 start;
    Vector2 end;
    PhysicsService* physics;
    BodyComponent* body;
    AnimationController* animation;
    EnemyType type;
    float radius = 12.0f;

    Enemy(EnemyType type, Vector2 start, Vector2 end) : type(type), start(start), end(end) {}
    void init_object() override
    {
        physics = scene->get_service<PhysicsService>();

        body = add_component<BodyComponent>(
            [=](BodyComponent& b)
            {
                b2BodyDef body_def = b2DefaultBodyDef();
                body_def.type = b2_kinematicBody;
                body_def.position = physics->convert_to_meters(start);
                body_def.userData = this;
                b.id = b2CreateBody(physics->world, &body_def);

                b2SurfaceMaterial body_material = b2DefaultSurfaceMaterial();

                b2ShapeDef circle_shape_def = b2DefaultShapeDef();
                circle_shape_def.density = 1.0f;
                circle_shape_def.material = body_material;
                circle_shape_def.isSensor = true;
                circle_shape_def.enableSensorEvents = true;

                b2Circle circle_shape = {b2Vec2_zero, physics->convert_to_meters(radius)};
                b2CreateCircleShape(b.id, &circle_shape_def, &circle_shape);
            });

        animation = add_component<AnimationController>(body);
        if (type == Bat)
        {
            animation->add_animation("move",
                                     std::vector<std::string>{"assets/pixel_platformer/enemies/bat_1.png",
                                                              "assets/pixel_platformer/enemies/bat_2.png",
                                                              "assets/pixel_platformer/enemies/bat_3.png"},
                                     5.0f);
        }
        else if (type == DrillHead)
        {
            animation->add_animation("move",
                                     std::vector<std::string>{"assets/pixel_platformer/enemies/drill_head_1.png",
                                                              "assets/pixel_platformer/enemies/drill_head_2.png"},
                                     5.0f);
        }
        else if (type == BlockHead)
        {
            animation->add_animation("move",
                                     std::vector<std::string>{"assets/pixel_platformer/enemies/block_head_1.png",
                                                              "assets/pixel_platformer/enemies/block_head_2.png"},
                                     5.0f);
        }
        animation->play("move");

        GameObject::init_object();

        // Start moving towards end position.
        Vector2 to_end = end - body->get_position_pixels();
        to_end = Vector2Normalize(to_end);
        body->set_velocity(to_end * 50.0f);
    }

    void update(float delta_time) override
    {
        b2Circle circle_shape = {body->get_position_meters(), physics->convert_to_meters(radius * 2.0f)};
        if (b2PointInCircle(physics->convert_to_meters(end), &circle_shape))
        {
            // Move towards start position.
            Vector2 to_start = start - body->get_position_pixels();
            to_start = Vector2Normalize(to_start);
            body->set_velocity(to_start * 50.0f);
        }
        else if (b2PointInCircle(physics->convert_to_meters(start), &circle_shape))
        {
            // Move towards end position.
            Vector2 to_end = end - body->get_position_pixels();
            to_end = Vector2Normalize(to_end);
            body->set_velocity(to_end * 50.0f);
        }

        // Check for collisions.
        auto sensor_contacts = body->get_sensor_overlaps();
        for (auto contact_body_id : sensor_contacts)
        {
            auto user_data = static_cast<GameObject*>(b2Body_GetUserData(contact_body_id));
            if (user_data && user_data->has_tag("character"))
            {
                // Hit player.
                CollectingCharacter* character = static_cast<CollectingCharacter*>(user_data);
                character->die();
            }
        }

        // Flip based on velocity.
        Vector2 velocity = body->get_velocity_pixels();
        animation->flip_x = velocity.x > 0.0f;
    }
};

/**
 * A collectible coin.
 */
class Coin : public GameObject
{
public:
    Vector2 position;
    PhysicsService* physics;
    BodyComponent* body;
    AnimationController* animation;
    SoundComponent* collect_sound;

    Coin(Vector2 position) : position(position) {}
    void init() override
    {
        physics = scene->get_service<PhysicsService>();

        body = add_component<BodyComponent>(
            [=](BodyComponent& b)
            {
                b2BodyDef body_def = b2DefaultBodyDef();
                body_def.type = b2_staticBody;
                body_def.position = physics->convert_to_meters(position);
                body_def.userData = this;
                b.id = b2CreateBody(physics->world, &body_def);

                b2SurfaceMaterial body_material = b2DefaultSurfaceMaterial();

                b2ShapeDef circle_shape_def = b2DefaultShapeDef();
                circle_shape_def.density = 1.0f;
                circle_shape_def.material = body_material;
                circle_shape_def.isSensor = true;
                circle_shape_def.enableSensorEvents = true;

                b2Circle circle_shape = {b2Vec2_zero, physics->convert_to_meters(8.0f)};
                b2CreateCircleShape(b.id, &circle_shape_def, &circle_shape);
            });

        animation = add_component<AnimationController>(body);
        animation->add_animation("spin",
                                 std::vector<std::string>{"assets/pixel_platformer/items/coin_1.png",
                                                          "assets/pixel_platformer/items/coin_2.png"},
                                 5.0f);
        animation->play("spin");

        collect_sound = add_component<SoundComponent>("assets/sounds/coin.wav");
    }

    void update(float delta_time) override
    {
        auto sensor_contacts = body->get_sensor_overlaps();
        for (auto contact_body_id : sensor_contacts)
        {
            auto user_data = static_cast<GameObject*>(b2Body_GetUserData(contact_body_id));
            if (user_data && user_data->has_tag("character"))
            {
                // Collected by character.
                collect_sound->play();

                // Disable the coin.
                is_active = false;
                body->disable();

                // Increase the score on the character.
                CollectingCharacter* character = static_cast<CollectingCharacter*>(user_data);
                character->score += 1;
                break;
            }
        }
    }
};

/**
 * A collecting game scene.
 */
class CollectingScene : public Scene
{
public:
    WindowManager* window_manager;
    FontManager* font_manager;
    std::vector<std::shared_ptr<CollectingCharacter>> characters;
    LevelService* level;
    PhysicsService* physics;
    std::vector<std::shared_ptr<SplitCamera>> cameras;
    Vector2 screen_size;
    float scale = 2.5f;

    void init_services() override
    {
        // TextureService and SoundService are needed by other components and game objects.
        add_service<TextureService>();
        add_service<SoundService>();

        // PhysicsService is used by LevelService and must be added first.
        physics = add_service<PhysicsService>();
        // Setup LDtk level. Checkout the file in LDtk editor to see how it's built.
        std::vector<std::string> collision_names = {"walls", "clouds", "trees"};
        level = add_service<LevelService>("assets/levels/collecting.ldtk", "Level", collision_names);
    }

    void init() override
    {
        window_manager = game->get_manager<WindowManager>();
        font_manager = game->get_manager<FontManager>();

        const auto& entities_layer = level->get_layer_by_name("Entities");

        // Create player characters at the "Start" entities.
        auto player_entities = level->get_entities_by_name("Start");

        for (int i = 0; i < player_entities.size() && i < 4; i++)
        {
            auto& player_entity = player_entities[i];
            CharacterParams params;
            params.position = level->convert_to_pixels(player_entity->getPosition());
            params.width = 16;
            params.height = 24;
            auto character = add_game_object<CollectingCharacter>(params, i + 1);
            character->add_tag("character");
            characters.push_back(character);
        }

        // Create enemies at the each enemy entity.
        auto bat_entities = level->get_entities_by_name("Bat");
        for (auto& bat_entity : bat_entities)
        {
            auto start_point = bat_entity->getPosition();
            Vector2 start_position = level->convert_to_pixels(start_point);
            ldtk::IntPoint end_point = bat_entity->getField<ldtk::IntPoint>("end").value();
            // Annoyingly, Point fields in LDtk are in cell coordinates rather than pixel coordinates, and the cell
            // size is dependent on the layer.
            Vector2 end_position = level->convert_cells_to_pixels(end_point, entities_layer);
            auto enemy = add_game_object<Enemy>(EnemyType::Bat, start_position, end_position);
            enemy->add_tag("enemy");
        }

        auto drill_entities = level->get_entities_by_name("DrillHead");
        for (auto& drill_entity : drill_entities)
        {
            auto start_point = drill_entity->getPosition();
            Vector2 start_position = level->convert_to_pixels(start_point);
            ldtk::IntPoint end_point = drill_entity->getField<ldtk::IntPoint>("end").value();
            Vector2 end_position = level->convert_cells_to_pixels(end_point, entities_layer);
            auto enemy = add_game_object<Enemy>(EnemyType::DrillHead, start_position, end_position);
            enemy->add_tag("enemy");
        }

        auto block_entities = level->get_entities_by_name("BlockHead");
        for (auto& block_entity : block_entities)
        {
            auto start_point = block_entity->getPosition();
            Vector2 start_position = level->convert_to_pixels(start_point);
            ldtk::IntPoint end_point = block_entity->getField<ldtk::IntPoint>("end").value();
            Vector2 end_position = level->convert_cells_to_pixels(end_point, entities_layer);
            auto enemy = add_game_object<Enemy>(EnemyType::BlockHead, start_position, end_position);
            enemy->add_tag("enemy");
        }

        // Create coins at the "Coin" entities.
        auto coin_entities = level->get_entities_by_name("Coin");
        for (auto& coin_entity : coin_entities)
        {
            Vector2 coin_position = level->convert_to_pixels(coin_entity->getPosition());
            auto coin = add_game_object<Coin>(coin_position);
            coin->add_tag("coin");
        }

        // Setup cameras.
        screen_size =
            Vector2{static_cast<float>(window_manager->get_width()), static_cast<float>(window_manager->get_height())};
        for (int i = 0; i < characters.size(); i++)
        {
            auto cam = add_game_object<SplitCamera>(screen_size / scale, level->get_size());
            cameras.push_back(cam);
        }
    }

    void update(float delta_time) override
    {
        // Set the camera target to follow each character.
        for (int i = 0; i < cameras.size(); i++)
        {
            cameras[i]->target = characters[i]->body->get_position_pixels();
        }

        auto new_screen_size = Vector2{static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
        if (new_screen_size != screen_size)
        {
            // Window resized, update cameras.
            screen_size = new_screen_size;
            // Scale the cameras so they are zoomed in when drawn.
            float screen_scale = window_manager->get_width() / screen_size.x;
            for (auto camera : cameras)
            {
                camera->size = screen_size / scale * screen_scale;
                camera->camera.offset = {camera->size.x / 2.0f, camera->size.y / 2.0f};
                UnloadRenderTexture(camera->renderer);
                camera->renderer = LoadRenderTexture((int)camera->size.x, (int)camera->size.y);
            }
        }

        // Trigger scene change on Enter key or gamepad start button.
        if (IsKeyPressed(KEY_ENTER) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT))
        {
            game->go_to_scene_next();
        }
    }

    /**
     * We override draw_scene instead of draw to control when Scene::draw_scene is called.
     * This allows us to draw the scene inside the camera's Begin/End block and render the camera to a texture.
     */
    void draw_scene() override
    {
        // The scene needs to be rendered once per camera.
        for (auto camera : cameras)
        {
            camera->draw_begin();
            Scene::draw_scene();
            // physics->draw_debug();
            // camera->draw_debug();
            camera->draw_end();
        }

        // Draw the cameras.
        ClearBackground(MAGENTA);

        // Draw each camera's texture side by side.
        for (int i = 0; i < cameras.size(); i++)
        {
            if (i == 0)
            {
                cameras[i]->draw_texture_pro(0, 0, screen_size.x / 2.0f, screen_size.y / 2.0f);
                DrawTextEx(font_manager->get_font("Tiny5"),
                           TextFormat("Score: %d", characters[0]->score),
                           Vector2{20.0f, 20.0f},
                           40.0f,
                           2.0f,
                           BLACK);
            }
            else if (i == 1)
            {
                cameras[i]->draw_texture_pro(screen_size.x / 2.0f, 0, screen_size.x / 2.0f, screen_size.y / 2.0f);
                DrawTextEx(font_manager->get_font("Tiny5"),
                           TextFormat("Score: %d", characters[1]->score),
                           Vector2{screen_size.x / 2.0f + 20.0f, 20.0f},
                           40.0f,
                           2.0f,
                           BLACK);
            }
            else if (i == 2)
            {
                cameras[i]->draw_texture_pro(0, screen_size.y / 2.0f, screen_size.x / 2.0f, screen_size.y / 2.0f);
                DrawTextEx(font_manager->get_font("Tiny5"),
                           TextFormat("Score: %d", characters[2]->score),
                           Vector2{20.0f, screen_size.y / 2.0f + 20.0f},
                           40.0f,
                           2.0f,
                           BLACK);
            }
            else if (i == 3)
            {
                cameras[i]->draw_texture_pro(screen_size.x / 2.0f,
                                             screen_size.y / 2.0f,
                                             screen_size.x / 2.0f,
                                             screen_size.y / 2.0f);
                DrawTextEx(font_manager->get_font("Tiny5"),
                           TextFormat("Score: %d", characters[3]->score),
                           Vector2{screen_size.x / 2.0f + 20.0f, screen_size.y / 2.0f + 20.0f},
                           40.0f,
                           2.0f,
                           BLACK);
            }
        }

        // Draw split lines.
        DrawLineEx(Vector2{screen_size.x / 2.0f, 0}, Vector2{screen_size.x / 2.0f, screen_size.y}, 4.0f, GRAY);
        DrawLineEx(Vector2{0, screen_size.y / 2.0f}, Vector2{screen_size.x, screen_size.y / 2.0f}, 4.0f, GRAY);
    }
};
