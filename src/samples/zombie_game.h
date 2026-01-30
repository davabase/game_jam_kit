/**
 * Demonstration of a top down shooter game.
 * Shows how to draw lights using custom blend modes.
 */

#pragma once

#include "engine/prefabs/includes.h"
#include "rlgl.h"

// Custom Blend Modes for lights. See https://www.raylib.com/examples/shapes/loader.html?name=shapes_top_down_lights
#define RLGL_SRC_ALPHA 0x0302
#define RLGL_MIN 0x8007

class ZombieScene;

/**
 * A bullet fired by a character.
 */
class Bullet : public GameObject
{
public:
    PhysicsService* physics;
    BodyComponent* body;
    SpriteComponent* sprite;
    SoundComponent* hit_sound;
    float speed = 800.0f; // pixels per second

    void init() override
    {
        physics = scene->get_service<PhysicsService>();

        body = add_component<BodyComponent>(
            [=](BodyComponent& b)
            {
                b2BodyDef body_def = b2DefaultBodyDef();
                body_def.type = b2_dynamicBody;
                body_def.isBullet = true;
                // Start off-screen.
                body_def.position = physics->convert_to_meters(Vector2{-1000.0f, -1000.0f});
                body_def.userData = this;
                b.id = b2CreateBody(physics->world, &body_def);

                b2SurfaceMaterial body_material = b2DefaultSurfaceMaterial();
                body_material.restitution = 0.0f;
                body_material.friction = 0.0f;

                b2ShapeDef circle_shape_def = b2DefaultShapeDef();
                circle_shape_def.density = 0.25f;
                circle_shape_def.material = body_material;

                b2Circle circle_shape = {b2Vec2_zero, physics->convert_to_meters(8.0f)};
                b2CreateCircleShape(b.id, &circle_shape_def, &circle_shape);
            });

        sprite = add_component<SpriteComponent>("assets/zombie_shooter/bullet.png", body);

        hit_sound = add_component<SoundComponent>("assets/sounds/hit.wav");
    }

    void update(float delta_time) override
    {
        auto contacts = body->get_contacts();
        for (const auto& contact : contacts)
        {
            // Deactivate the bullet if we hit anything.
            is_active = false;
            // Move it off-screen.
            body->set_position(Vector2{-1000.0f, -1000.0f});
            body->set_velocity(Vector2{0.0f, 0.0f});

            GameObject* other = static_cast<GameObject*>(b2Body_GetUserData(contact));
            if (other)
            {
                if (other->has_tag("zombie"))
                {
                    hit_sound->play();

                    // Hit a zombie, deactivate it too.
                    other->is_active = false;
                    // Move it off-screen.
                    auto zombie_body = other->get_component<BodyComponent>();
                    if (zombie_body)
                    {
                        zombie_body->set_position(Vector2{-1000.0f, -1000.0f});
                        zombie_body->set_velocity(Vector2{0.0f, 0.0f});
                        zombie_body->disable();
                    }
                    auto zombie_sprite = other->get_component<SpriteComponent>();
                    if (zombie_sprite)
                    {
                        zombie_sprite->set_position(Vector2{-1000.0f, -1000.0f});
                    }
                }
                break;
            }
        }
    }
};

/**
 * A top-down character controlled by the player.
 */
class TopDownCharacter : public GameObject
{
public:
    Vector2 position = {0, 0};
    BodyComponent* body;
    PhysicsService* physics;
    SpriteComponent* sprite;
    TopDownMovementComponent* movement;
    MultiComponent<SoundComponent>* sounds;
    SoundComponent* shoot_sound;
    std::vector<std::shared_ptr<Bullet>> bullets;
    int player_num = 0;
    int health = 10;
    float contact_timer = 1.0f;
    float contact_cooldown = 0.3f;

    TopDownCharacter(Vector2 position, std::vector<std::shared_ptr<Bullet>> bullets, int player_num = 0) :
        position(position),
        bullets(std::move(bullets)),
        player_num(player_num)
    {
    }

    void init() override
    {
        // Grab the physics service.
        // All get_service calls should be done in init(). get_service is not quick and this also allows us to test
        // that all services exist during init time.
        physics = scene->get_service<PhysicsService>();

        body = add_component<BodyComponent>(
            [=](BodyComponent& b)
            {
                b2BodyDef body_def = b2DefaultBodyDef();
                body_def.type = b2_dynamicBody;
                body_def.fixedRotation = true;
                body_def.position = physics->convert_to_meters(position);
                body_def.userData = this;
                b.id = b2CreateBody(physics->world, &body_def);

                b2SurfaceMaterial body_material = b2DefaultSurfaceMaterial();

                b2ShapeDef circle_shape_def = b2DefaultShapeDef();
                circle_shape_def.density = 1.0f;
                circle_shape_def.material = body_material;

                b2Circle circle_shape = {b2Vec2_zero, physics->convert_to_meters(16.0f)};
                b2CreateCircleShape(b.id, &circle_shape_def, &circle_shape);
            });

        // Setup movement.
        TopDownMovementParams mp;
        mp.accel = 5000.0f;
        mp.friction = 5000.0f;
        mp.max_speed = 350.0f;
        movement = add_component<TopDownMovementComponent>(mp);

        // Setup sounds.
        // TODO: Is only allowing one component per type really as cool an idea as I thought?
        sounds = add_component<MultiComponent<SoundComponent>>();
        shoot_sound = sounds->add_component("shoot", "assets/sounds/shoot.wav");

        // Setup sprite.
        sprite =
            add_component<SpriteComponent>("assets/zombie_shooter/player_" + std::to_string(player_num + 1) + ".png");
    }

    void update(float delta_time) override
    {
        Vector2 move = {0.0f, 0.0f};

        move = {GetGamepadAxisMovement(player_num, GAMEPAD_AXIS_LEFT_X),
                GetGamepadAxisMovement(player_num, GAMEPAD_AXIS_LEFT_Y)};

        if (IsKeyDown(KEY_W) || IsGamepadButtonDown(player_num, GAMEPAD_BUTTON_LEFT_FACE_UP))
        {
            move.y -= 1.0f;
        }
        if (IsKeyDown(KEY_S) || IsGamepadButtonDown(player_num, GAMEPAD_BUTTON_LEFT_FACE_DOWN))
        {
            move.y += 1.0f;
        }
        if (IsKeyDown(KEY_A) || IsGamepadButtonDown(player_num, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
        {
            move.x -= 1.0f;
        }
        if (IsKeyDown(KEY_D) || IsGamepadButtonDown(player_num, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
        {
            move.x += 1.0f;
        }

        movement->set_input(move.x, move.y);

        // Update sprite position and rotation.
        sprite->set_position(body->get_position_pixels());
        sprite->set_rotation(movement->facing_dir);

        // Shooting
        if (IsKeyPressed(KEY_SPACE) || IsGamepadButtonPressed(player_num, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
        {
            // Find an inactive bullet from the pool.
            for (auto& bullet : bullets)
            {
                if (!bullet->is_active)
                {
                    // Play shoot sound.
                    shoot_sound->play();

                    // Activate and position the bullet.
                    Vector2 char_pos = body->get_position_pixels();
                    Vector2 shoot_dir = {std::cos(movement->facing_dir * DEG2RAD),
                                         std::sin(movement->facing_dir * DEG2RAD)};
                    Vector2 bullet_start_pos = {char_pos.x + shoot_dir.x * 48.0f, char_pos.y + shoot_dir.y * 48.0f};
                    bullet->body->set_position(bullet_start_pos);

                    bullet->body->set_rotation(movement->facing_dir + 90.0f);

                    // Set bullet velocity.
                    Vector2 velocity = {shoot_dir.x * bullet->speed, shoot_dir.y * bullet->speed};
                    bullet->body->set_velocity(velocity);
                    bullet->is_active = true;
                    break;
                }
            }
        }

        // Damage.
        auto contacts = body->get_contacts();
        for (const auto& contact : contacts)
        {
            GameObject* other = static_cast<GameObject*>(b2Body_GetUserData(contact));
            if (other)
            {
                if (other->has_tag("zombie"))
                {
                    // When we are in contact with a zombie long enough, take 1 damage.
                    if (contact_timer > 0.0f)
                    {
                        contact_timer -= delta_time;
                    }
                    if (contact_timer <= 0.0f)
                    {
                        health -= 1;
                        contact_timer = contact_cooldown;
                        if (health <= 0)
                        {
                            // Deactivate character.
                            is_active = false;
                            // Move off-screen.
                            body->set_position(Vector2{-1000.0f, -1000.0f});
                            body->set_velocity(Vector2{0.0f, 0.0f});
                        }
                    }
                }
            }
        }
    }
};

/**
 * A zombie that chases the closest player.
 */
class Zombie : public GameObject
{
public:
    BodyComponent* body;
    PhysicsService* physics;
    SpriteComponent* sprite;
    TopDownMovementComponent* movement;
    std::vector<std::shared_ptr<TopDownCharacter>> players;

    Zombie(std::vector<std::shared_ptr<TopDownCharacter>> players) : players(std::move(players)) {}

    void init() override
    {
        // Grab the physics service.
        physics = scene->get_service<PhysicsService>();

        body = add_component<BodyComponent>(
            [=](BodyComponent& b)
            {
                b2BodyDef body_def = b2DefaultBodyDef();
                body_def.type = b2_dynamicBody;
                body_def.fixedRotation = true;
                body_def.position = physics->convert_to_meters(Vector2{-1000.0f, -1000.0f});
                body_def.userData = this;
                b.id = b2CreateBody(physics->world, &body_def);

                b2SurfaceMaterial body_material = b2DefaultSurfaceMaterial();

                b2ShapeDef circle_shape_def = b2DefaultShapeDef();
                circle_shape_def.density = 1.0f;
                circle_shape_def.material = body_material;

                b2Circle circle_shape = {b2Vec2_zero, physics->convert_to_meters(16.0f)};
                b2CreateCircleShape(b.id, &circle_shape_def, &circle_shape);
                // Disable by default.
                b2Body_Disable(b.id);
            });

        // Setup movement.
        TopDownMovementParams mp;
        mp.accel = 5000.0f;
        mp.friction = 5000.0f;
        mp.max_speed = 100.0f;
        movement = add_component<TopDownMovementComponent>(mp);

        // Setup sprite.
        sprite = add_component<SpriteComponent>("assets/zombie_shooter/zombie.png");
    }

    void update(float delta_time) override
    {
        // Find the closest player and move towards them.
        Vector2 closest_player_pos = {0, 0};
        float closest_dist_sq = FLT_MAX;
        for (auto& player : players)
        {
            Vector2 player_pos = player->body->get_position_pixels();
            Vector2 to_player = {player_pos.x - body->get_position_pixels().x,
                                 player_pos.y - body->get_position_pixels().y};
            float dist_sq = to_player.x * to_player.x + to_player.y * to_player.y;
            if (dist_sq < closest_dist_sq)
            {
                closest_dist_sq = dist_sq;
                closest_player_pos = player_pos;
            }
        }
        Vector2 to_closest = {closest_player_pos.x - body->get_position_pixels().x,
                              closest_player_pos.y - body->get_position_pixels().y};
        float to_closest_len = std::sqrt(to_closest.x * to_closest.x + to_closest.y * to_closest.y);
        if (to_closest_len > 0.0f)
        {
            to_closest.x /= to_closest_len;
            to_closest.y /= to_closest_len;
        }
        movement->set_input(to_closest.x, to_closest.y);

        // Update sprite position and rotation.
        sprite->set_position(body->get_position_pixels());
        sprite->set_rotation(movement->facing_dir);
    }
};

/**
 * A spawner that spawns zombies at intervals.
 */
class Spawner : public GameObject
{
public:
    float spawn_timer = 0.0f;
    float spawn_interval = 1.0f; // Spawn a zombie every 1 second
    std::vector<std::shared_ptr<Zombie>> zombie_pool;
    Vector2 position = {0, 0};
    Vector2 size = {0, 0};

    Spawner(Vector2 position, Vector2 size, std::vector<std::shared_ptr<Zombie>> zombies) :
        position(position - size * 0.5f),
        size(size),
        zombie_pool(std::move(zombies))
    {
    }

    void update(float delta_time) override
    {
        spawn_timer -= delta_time;
        if (spawn_timer <= 0.0f)
        {
            spawn_timer = spawn_interval;

            // Spawn a zombie at a random position within the spawner area
            float x = position.x + static_cast<float>(GetRandomValue(0, static_cast<int>(size.x)));
            float y = position.y + static_cast<float>(GetRandomValue(0, static_cast<int>(size.y)));
            Vector2 spawn_pos = {x, y};

            for (auto& zombie : zombie_pool)
            {
                if (!zombie->is_active)
                {
                    zombie->body->set_position(spawn_pos);
                    zombie->is_active = true;
                    zombie->body->enable();
                    return;
                }
            }
        }
    }
};

/**
 * A scene for the zombie shooter game.
 */
class ZombieScene : public Scene
{
public:
    FontManager* font_manager;
    PhysicsService* physics;
    LevelService* level;
    RenderTexture2D renderer;
    RenderTexture2D light_map;
    Texture2D light_texture;
    std::vector<std::shared_ptr<Bullet>> bullets;
    std::vector<std::shared_ptr<TopDownCharacter>> characters;
    std::vector<std::shared_ptr<Zombie>> zombies;

    void init_services() override
    {
        // TextureService and SoundService are needed by other components and game objects.
        add_service<TextureService>();
        add_service<SoundService>();

        // Set gravity to zero for top-down game.
        physics = add_service<PhysicsService>(b2Vec2_zero);
        std::vector<std::string> collision_names = {"walls", "obstacles"};
        level = add_service<LevelService>("assets/levels/top_down.ldtk", "Level", collision_names);

        // Grab the font manager.
        font_manager = game->get_manager<FontManager>();
    }

    void init() override
    {
        const auto& entities_layer = level->get_layer_by_name("Entities");

        // Prepare a pool of bullets.
        // It is unwise to call init during update loops, so we create all bullets here and deactivate them.
        for (int i = 0; i < 100; i++)
        {
            auto bullet = add_game_object<Bullet>();
            // When is_active is false, update and draw are skipped.
            bullet->is_active = false;
            bullets.push_back(bullet);
        }

        // Create player characters.
        auto player_entities = level->get_entities_by_name("Start");

        for (int i = 0; i < player_entities.size() && i < 4; i++)
        {
            auto& player_entity = player_entities[i];
            auto position = level->convert_to_pixels(player_entity->getPosition());
            auto character = add_game_object<TopDownCharacter>(position, bullets, i);
            character->add_tag("player");
            characters.push_back(character);
        }

        // Prepare a pool of zombies.
        // It is unwise to call init during update loops, so we create all zombies here and deactivate them.
        for (int i = 0; i < 100; i++)
        {
            // Start off-screen
            auto zombie = add_game_object<Zombie>(characters);
            // When is_active is false, update and draw are skipped.
            zombie->is_active = false;
            zombie->add_tag("zombie");
            zombies.push_back(zombie);
        }

        // Create spawner.
        auto spawn_entity = level->get_entities_by_name("Spawn")[0];
        auto spawn_position = level->convert_to_pixels(spawn_entity->getPosition());
        auto spawn_size = level->convert_to_pixels(spawn_entity->getSize());
        auto spawner = add_game_object<Spawner>(spawn_position, spawn_size, zombies);

        // We want to control when the foreground layer is drawn.
        level->set_layer_visibility("Foreground", false);

        // Create render texture to scale the level to the screen.
        renderer = LoadRenderTexture((int)level->get_size().x, (int)level->get_size().y);
        light_map = LoadRenderTexture((int)level->get_size().x, (int)level->get_size().y);

        light_texture = get_service<TextureService>()->get_texture("assets/zombie_shooter/light.png");
    }

    void update(float delta_time) override
    {
        // Trigger scene change on Enter key or gamepad start button.
        if (IsKeyPressed(KEY_ENTER) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT))
        {
            game->go_to_scene_next();
        }
    }

    void draw_scene() override
    {
        // Build up the light mask
        BeginTextureMode(light_map);
        ClearBackground(BLACK);

        // Force the blend mode to only set the alpha of the destination
        rlSetBlendFactors(RLGL_SRC_ALPHA, RLGL_SRC_ALPHA, RLGL_MIN);
        rlSetBlendMode(BLEND_CUSTOM);

        // Merge in all the light masks
        for (int i = 0; i < 4; i++)
        {
            // DrawCircleGradient((int)characters[i]->body->get_position_pixels().x,
            //                    (int)characters[i]->body->get_position_pixels().y,
            //                    300.0f,
            //                    ColorAlpha(WHITE, 0),
            //                    ColorAlpha(BLACK, 1));
            DrawTexture(light_texture,
                        (int)(characters[i]->body->get_position_pixels().x - light_texture.width / 2),
                        (int)(characters[i]->body->get_position_pixels().y - light_texture.height / 2),
                        WHITE);
        }

        rlDrawRenderBatchActive();

        // Go back to normal blend
        rlSetBlendMode(BLEND_ALPHA);
        EndTextureMode();

        // Draw to render texture first.
        BeginTextureMode(renderer);
        ClearBackground(MAGENTA);
        Scene::draw_scene();
        level->draw_layer("Foreground");
        DrawTexturePro(
            light_map.texture,
            {0.0f, 0.0f, static_cast<float>(light_map.texture.width), static_cast<float>(-light_map.texture.height)},
            {0.0f, 0.0f, static_cast<float>(light_map.texture.width), static_cast<float>(light_map.texture.height)},
            {0.0f, 0.0f},
            0.0f,
            ColorAlpha(WHITE, 0.92f));
        DrawRectangle(10, 10, 210, 210, Fade(WHITE, 0.3f));
        DrawTextEx(font_manager->get_font("Roboto"),
                   TextFormat("Health: %d\nHealth: %d\nHealth: %d\nHealth: %d",
                              characters[0]->health,
                              characters[1]->health,
                              characters[2]->health,
                              characters[3]->health),
                   Vector2{20.0f, 20.0f},
                   45.0f,
                   1.0f,
                   RED);
        EndTextureMode();

        // Draw the render texture scaled to the screen.
        DrawTexturePro(
            renderer.texture,
            {0.0f, 0.0f, static_cast<float>(renderer.texture.width), static_cast<float>(-renderer.texture.height)},
            {0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
            {0.0f, 0.0f},
            0.0f,
            WHITE);
    }
};
