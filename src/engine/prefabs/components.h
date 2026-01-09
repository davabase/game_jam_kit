#pragma once

#include "engine/raycasts.h"
#include "engine/framework.h"
#include "engine/prefabs/managers.h"
#include "engine/prefabs/services.h"

/**
 * For when you want a GameObject to have multiple of the same component.
 */
template <typename T>
class MultiComponent : public Component {
public:
    std::unordered_map<std::string, std::unique_ptr<T>> components;

    MultiComponent() {}

    void init() override {
        for (auto& component : components) {
            component.second->init();
        }
    }

    void update(float delta_time) override {
        for (auto& component : components) {
            component.second->update(delta_time);
        }
    }

    void draw() override {
        for (auto& component : components) {
            component.second->draw();
        }
    }

    void add_component(std::string name, std::unique_ptr<T> component) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        components[name] = std::move(component);
    }

    template <typename... TArgs>
    T* add_component(std::string name, TArgs&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        auto new_component = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* component_ptr = new_component.get();
        add_component(name, std::move(new_component));
        return component_ptr;
    }

    T* get_component(std::string name) {
        return components[name].get();
    }
};

class TextComponent : public Component {
public:
    FontManager* font_manager;
    std::string text;
    std::string font_name;
    int font_size = 20;
    Color color = WHITE;
    Vector2 position = {0, 0};
    float rotation = 0.0f;

    void init() override {
        font_manager = owner->scene->get_manager<FontManager>();
    }

    TextComponent(std::string text, std::string font_name = "default", int font_size = 20, Color color = WHITE)
        : text(text), font_name(font_name), font_size(font_size), color(color) {}

    void draw() override {
        DrawTextEx(font_manager->get_font(font_name), text.c_str(), position, static_cast<float>(font_size), 1.0f, color);
    }
};

class SoundComponent : public Component {
public:
    std::string filename;
    Sound sound;
    float volume = 1.0f;
    float pitch = 1.0f;

    SoundComponent(std::string filename, float volume = 1.0f, float pitch = 1.0f)
        : filename(filename), volume(volume), pitch(pitch) {}

    void init() override {
        auto sound_service = owner->scene->get_service<SoundService>();
        sound = sound_service->get_sound(filename);
    }

    void play() {
        PlaySound(sound);
    }

    void stop() {
        StopSound(sound);
    }

    void set_volume(float volume) {
        this->volume = volume;
        SetSoundVolume(sound, volume);
    }

    void set_pitch(float pitch) {
        this->pitch = pitch;
        SetSoundPitch(sound, pitch);
    }

    bool is_playing() {
        return IsSoundPlaying(sound);
    }
};

class BodyComponent : public Component {
public:
    b2BodyId id = b2_nullBodyId;
    std::function<void(BodyComponent&)> build;
    PhysicsService* physics;

    BodyComponent() {}
    BodyComponent(b2BodyId id) : id(id) {}

    /**
     * Specify a lambda for creating the physics body which will be called during init.
     * It is the user's responsibility to assign the body id to id in this function.
     *
     * @param build A user provided function for creating a physics body.
     */
    BodyComponent(std::function<void(BodyComponent&)> build = {}) : build(std::move(build)) {}

    ~BodyComponent() {
        if (b2Body_IsValid(id)) {
            b2DestroyBody(id);
        }
    }

    void init() override {
        physics = owner->scene->get_service<PhysicsService>();
        if (build) {
            build(*this);
        }
    }

    b2Vec2 get_position_meters() const {
        return b2Body_GetPosition(id);
    }

    Vector2 get_position_pixels() const {
        return physics->convert_to_pixels(get_position_meters());
    }

    b2Vec2 get_velocity_meters() const {
        return b2Body_GetLinearVelocity(id);
    }

    Vector2 get_velocity_pixels() const {
        return physics->convert_to_pixels(get_velocity_meters());
    }

    void set_velocity(b2Vec2 meters_per_second) {
        b2Body_SetLinearVelocity(id, meters_per_second);
    }

    void set_velocity(Vector2 pixels_per_second) {
        set_velocity(physics->convert_to_meters(pixels_per_second));
    }

    float get_rotation() const {
        auto rot = b2Body_GetRotation(id);
        return b2Rot_GetAngle(rot) * RAD2DEG;
    }

    /**
     * Get a list of all bodies colliding with this one.
     *
     * @return A list of b2BodyIds that are touching this one. Combine with User Data to get your objects.
     */
    std::vector<b2BodyId> get_contacts() {
        int capacity = b2Body_GetContactCapacity(id);
        std::vector<b2ContactData> contact_data;
        contact_data.reserve(capacity);

        int count = b2Body_GetContactData(id, contact_data.data(), capacity);
        std::vector<b2BodyId> contacts;
        for (int i = 0; i < count; i++) {
            auto contact = contact_data[i];
            auto body_a = b2Shape_GetBody(contact.shapeIdA);
            auto body_b = b2Shape_GetBody(contact.shapeIdB);
            auto body = body_a == id ? body_b : body_a;
            contacts.push_back(body);
        }

        // Remove duplicate bodies.
        std::sort(contacts.begin(), contacts.end());
        contacts.erase(std::unique(contacts.begin(), contacts.end() ), contacts.end());
        return contacts;
    }

    /**
     * Get a list of all bodies overlapping with sensors in this body.
     * The shape definitions must have isSensor and enableSensorEvents set. https://box2d.org/documentation/md_simulation.html#autotoc_md81
     *
     * @return A list of b2BodyIds that are overlapping the sensor shapes in this body. Combine with User Data to get your objects.
     */
    std::vector<b2BodyId> get_sensor_overlaps() {
        int shape_capacity = b2Body_GetShapeCount(id);
        std::vector<b2ShapeId> shapes;
        shapes.reserve(shape_capacity);
        int shape_count = b2Body_GetShapes(id, shapes.data(), shape_capacity);

        std::vector<b2BodyId> contacts;
        for (int i = 0; i < shape_count; i++) {
            auto shape = shapes[i];
            if (b2Shape_IsSensor(shape)) {
                int contact_capacity = b2Shape_GetContactCapacity(shape);
                std::vector<b2ContactData> contact_data;
                contact_data.reserve(contact_capacity);
                int contact_count = b2Shape_GetContactData(shape, contact_data.data(), contact_capacity);

                for (int j = 0; j < contact_count; j++) {
                    auto contact = contact_data[j];
                    auto body_a = b2Shape_GetBody(contact.shapeIdA);
                    auto body_b = b2Shape_GetBody(contact.shapeIdB);
                    auto body = body_a == id ? body_b : body_a;
                    contacts.push_back(body);
                }
            }
        }

        // Remove duplicate bodies.
        std::sort(contacts.begin(), contacts.end());
        contacts.erase(std::unique(contacts.begin(), contacts.end() ), contacts.end());
        return contacts;
    }
};

class SpriteComponent : public Component {
public:
    std::string filename;
    BodyComponent* body = nullptr;

    Texture2D sprite;
    Vector2 position = {0, 0};
    float rotation = 0.0f;
    float scale = 1.0f;
    Color tint = WHITE;
    bool is_active = true;

    SpriteComponent(std::string filename) : filename(filename) {}
    SpriteComponent(BodyComponent* body, std::string filename) : body(body), filename(filename) {}

    void init() override {
        auto texture_service = owner->scene->get_service<TextureService>();
        sprite = texture_service->get_texture(filename);
    }

    void draw() override {
        if (!is_active) {
            return;
        }

        if (body) {
            position = body->get_position_pixels();
            rotation = body->get_rotation();
        }

        Rectangle source = {0, 0, (float)sprite.width, (float)sprite.height};
        Rectangle dest = {position.x, position.y, (float)sprite.width * scale, (float)sprite.height * scale};
        Vector2 origin = {sprite.width / 2.0f * scale, sprite.height / 2.0f * scale};

        DrawTexturePro(sprite, source, dest, origin, rotation, tint);
    }

    void set_position(Vector2 position) {
        this->position = position;
    }

    void set_rotation(float rotation) {
        this->rotation = rotation;
    }

    void set_scale(float scale) {
        this->scale = scale;
    }

    void set_tint(Color tint) {
        this->tint = tint;
    }

    void set_active(bool active) {
        is_active = active;
    }
};

class Animation {
public:
    std::vector<Texture2D> frames;
    float fps = 15.0f;
    float frame_timer = 0.0f;
    bool loop = true;

    int current_frame = 0;
    bool playing = true;
    bool is_active = true;

    Animation(const std::vector<Texture2D>& frames, float fps = 15.0f, bool loop = true) : frames(frames), fps(fps), frame_timer(1.0f / fps), loop(loop) {}

    Animation(TextureService* texture_service, const std::vector<std::string>& filenames, float fps = 15.0f, bool loop = true) : fps(fps), frame_timer(1.0f / fps), loop(loop) {
        for (const auto& filename : filenames) {
            frames.push_back(texture_service->get_texture(filename));
        }
    }

    void update(float delta_time) {
        if (frames.empty()) {
            return;
        }
        if (!playing || !is_active) {
            return;
        }

        frame_timer -= delta_time;
        if (frame_timer <= 0.0f) {
            frame_timer = 1.0f / fps;
            current_frame++;
        }

        if (current_frame > frames.size() - 1) {
            if (loop)
                current_frame = 0;
            else {
                current_frame = frames.size() - 1;
            }
        }
    }

    void draw(Vector2 position, float rotation = 0.0f, Color tint = WHITE) {
        if (!is_active) {
            return;
        }

        auto sprite = frames[current_frame];
        DrawTexturePro(
            sprite,
            { 0.0f, 0.0f, static_cast<float>(sprite.width), static_cast<float>(sprite.height) },
            { position.x, position.y, static_cast<float>(sprite.width), static_cast<float>(sprite.height) },
            { static_cast<float>(sprite.width) / 2.0f, static_cast<float>(sprite.height) / 2.0f },
            rotation,
            tint
        );
    }

    void draw(Vector2 position, Vector2 origin, float rotation = 0.0f, float scale = 1.0f, bool flip_x = false, bool flip_y = false, Color tint = WHITE) {
        if (!is_active) {
            return;
        }

        auto sprite = frames[current_frame];
        DrawTexturePro(
            sprite,
            { 0.0f, 0.0f, static_cast<float>(sprite.width) * (flip_x ? -1.0f : 1.0f), static_cast<float>(sprite.height) * (flip_y ? -1.0f : 1.0f) },
            { position.x, position.y, static_cast<float>(sprite.width) * scale, static_cast<float>(sprite.height) * scale },
            origin * scale,
            rotation,
            tint
        );
    }

    void play() {
        playing = true;
    }

    void pause() {
        playing = false;
    }

    void stop() {
        playing = false;
        frame_timer = 1.0f / fps;
        current_frame = 0;
    }
};

class AnimationController : public Component {
public:
    std::unordered_map<std::string, std::unique_ptr<Animation>> animations;
    Animation* current_animation = nullptr;
    Vector2 position = {0.0f, 0.0f};
    float rotation = 0.0f;
    Vector2 origin = {0.0f, 0.0f};
    float scale = 1.0f;
    bool flip_x = false;
    bool flip_y = false;
    BodyComponent* body = nullptr;

    AnimationController() = default;

    AnimationController(BodyComponent* body) : body(body) {}

    void update(float delta_time) override {
        if (current_animation) {
            current_animation->update(delta_time);
        }
    }

    void draw() override {
        if (body) {
            position = body->get_position_pixels();
            rotation = body->get_rotation();
        }

        if (current_animation) {
            current_animation->draw(position, origin, rotation, scale, flip_x, flip_y);
        }
    }

    void add_animation(const std::string& name, std::unique_ptr<Animation> animation) {
        animations[name] = std::move(animation);
        if (!current_animation) {
            current_animation = animations[name].get();
        }
    }

    template <typename... TArgs>
    Animation* add_animation(const std::string& name, TArgs&&... args) {
        auto texture_service = owner->scene->get_service<TextureService>();
        auto new_animation = std::make_unique<Animation>(texture_service, std::forward<TArgs>(args)...);
        Animation* animation_ptr = new_animation.get();
        add_animation(name, std::move(new_animation));
        return animation_ptr;
    }

    Animation* get_animation(const std::string& name) {
        return animations[name].get();
    }

    void play() {
        if (current_animation) {
            current_animation->play();
        }
    }

    void play(const std::string& name) {
        auto it = animations.find(name);
        if (it != animations.end()) {
            current_animation = it->second.get();
            current_animation->play();
            auto sprite = current_animation->frames[current_animation->current_frame];
            origin = { sprite.width / 2.0f, sprite.height / 2.0f };
        }
    }

    void pause() {
        if (current_animation) {
            current_animation->pause();
        }
    }

    void set_play(bool play) {
        if (current_animation) {
            if (play) {
                current_animation->play();
            } else {
                current_animation->pause();
            }
        }
    }

    void stop() {
        if (current_animation) {
            current_animation->stop();
        }
    }

    void set_position(Vector2 pos) {
        position = pos;
    }

    void set_rotation(float rot) {
        rotation = rot;
    }

    void set_origin(Vector2 orig) {
        origin = orig;
    }

    void set_scale(float s) {
        scale = s;
    }

    void set_flip_x(bool fx) {
        flip_x = fx;
    }

    void set_flip_y(bool fy) {
        flip_y = fy;
    }
};

struct MovementParams {
    float width = 24.0f; // pixels
    float height = 40.0f; // pixels

    // Movement
    float max_speed = 220.0f; // pixels / second
    float accel = 2000.0f; // pixels / second / second
    float decel = 2500.0f; // pixels / second / second

    // Gravity / jump
    float gravity = 1400.0f; // pixels / second / second
    float jump_speed = 520.0f; // pixels / second
    float fall_speed = 1200.0f; // pixels / second
    float jump_cutoff_multiplier = 0.45f; // jump multiplier when the jump button is released early

    // Forgiveness
    float coyote_time = 0.08f; // seconds
    float jump_buffer = 0.10f; // seconds
};

class MovementComponent : public Component {
public:
    MovementParams p;
    PhysicsService* physics;
    BodyComponent* body;

    bool grounded = false;
    bool on_wall_left = false;
    bool on_wall_right = false;
    float coyote_timer = 0.0f;
    float jump_buffer_timer = 0.0f;

    float move_x = 0;
    bool jump_pressed = false;
    bool jump_held = false;

    MovementComponent(MovementParams p) : p(p) {}

    void init() override {
        physics = owner->scene->get_service<PhysicsService>();
        body = owner->get_component<BodyComponent>();
    }

    void update(float delta_time) override {
        if (!b2Body_IsValid(body->id)) {
            return;
        }

        coyote_timer = std::max(0.0f, coyote_timer - delta_time);
        jump_buffer_timer = std::max(0.0f, jump_buffer_timer - delta_time);

        if (jump_pressed) {
            jump_buffer_timer = p.jump_buffer;
        }

        // Grounded check
        grounded = false;
        on_wall_left = false;
        on_wall_right = false;

        // Convert probe distances to meters
        float ray_length = physics->convert_to_meters(4.0f);

        float half_width = physics->convert_to_meters(p.width) / 2.0f;
        float half_height = physics->convert_to_meters(p.height) / 2.0f;

        // Ground: cast down from two points near the feet (left/right)
        auto pos = body->get_position_meters();
        b2Vec2 ground_left_start = { pos.x - half_width, pos.y + half_height};
        b2Vec2 ground_right_start = { pos.x + half_width, pos.y + half_height};
        b2Vec2 ground_translation = { 0, ray_length };
        const b2WorldId world = physics->world;

        RayHit left_ground_hit = raycast_closest(world, body->id, ground_left_start, ground_translation);
        RayHit right_ground_hit = raycast_closest(world, body->id, ground_right_start, ground_translation);
        grounded = left_ground_hit.hit || right_ground_hit.hit;

        // Walls: cast left/right at mid-body height
        b2Vec2 mid = { pos.x, pos.y };
        b2Vec2 wall_left_start  = { pos.x - half_width, mid.y };
        b2Vec2 wall_left_translation = { -ray_length, 0 };
        b2Vec2 wall_right_start = { pos.x + half_width, mid.y };
        b2Vec2 wall_right_translation = { ray_length, 0 };

        RayHit left_wall_hit  = raycast_closest(world, body->id, wall_left_start, wall_left_translation);
        RayHit right_wall_hit = raycast_closest(world, body->id, wall_right_start, wall_right_translation);

        on_wall_left = left_wall_hit.hit;
        on_wall_right = right_wall_hit.hit;
        if (grounded) {
            coyote_timer = p.coyote_time;
        }

        float target_vx = move_x * p.max_speed;

        auto v = body->get_velocity_pixels();

        if (std::fabs(target_vx) > 0.001f) {
            float a = p.accel;
            v.x = move_towards(v.x, target_vx, a * delta_time);
        } else {
            float a = p.decel;
            v.x = move_towards(v.x, 0.0f, a * delta_time);
        }

        // Gravity (custom)
        v.y += p.gravity * delta_time;
        v.y = std::max(-p.fall_speed, std::min(p.fall_speed, v.y));

        // Jump
        const bool can_jump = (grounded || coyote_timer > 0.0f);
        if (jump_buffer_timer > 0.0f && can_jump) {
            v.y = -p.jump_speed;
            jump_buffer_timer = 0.0f;
            coyote_timer = 0.0f;
            grounded = false;
        }

        // Variable jump height: cut upward velocity when jump released
        if (!jump_held && v.y < 0.0f) {
            v.y *= p.jump_cutoff_multiplier;
        }

        // Write velocity back
        body->set_velocity(v);
    }

    static float move_towards(float current, float target, float max_delta) {
        float delta = target - current;
        if (std::fabs(delta) <= max_delta) return target;
        return current + (delta > 0 ? max_delta : -max_delta);
    }

    void set_input(float horizontal_speed, bool jump_pressed, bool jump_held) {
        move_x = horizontal_speed;
        this->jump_pressed = jump_pressed;
        this->jump_held = jump_held;
    }
};
