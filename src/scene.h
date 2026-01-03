#pragma once

#include <assert.h>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <box2d/box2d.h>
#include <LDtkLoader/Project.hpp>
#include <raylib.h>

#include "debug_draw.h"
#include "raycasts.h"

// Forward declarations.
class GameObject;
class Scene;

class Component {
public:
    GameObject* owner = nullptr;

    Component() = default;
    virtual ~Component() = default;

    virtual void init() {}
    virtual void load() {}
    virtual void update() {}
    virtual void draw() {}
};

class GameObject {
public:
    Scene* scene = nullptr;
    std::vector<std::unique_ptr<Component>> components;
    std::unordered_set<std::string> tags;

    GameObject() = default;
    virtual ~GameObject() = default;

    void add_component(std::unique_ptr<Component> component) {
        component->owner = this;
        components.push_back(std::move(component));
    }

    template <typename T, typename... TArgs>
    T* add_component(TArgs&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        auto new_object = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* object_ptr = new_object.get();
        add_component(std::move(new_object));
        return object_ptr;
    }

    void add_tag(const std::string& tag) {
        tags.insert(tag);
    }

    void remove_tag(const std::string& tag) {
        tags.erase(tag);
    }

    bool has_tag(const std::string& tag) const {
        return tags.find(tag) != tags.end();
    }

    virtual void init() {
        for (auto& component : components) {
            component->init();
        }
    }

    virtual void load() {
        for (auto& component : components) {
            component->load();
        }
    }

    virtual void update(float delta_time) {
        for (auto& component : components) {
            component->update();
        }
    }

    virtual void draw() {
        for (auto& component : components) {
            component->draw();
        }
    }
};

class Service {
public:
    Scene* scene = nullptr;
    bool is_init = false;

    Service() = default;
    virtual ~Service() = default;

    virtual void init() {
        is_init = true;
    }
    virtual void update() {}
    virtual void draw() {}
};

class Scene {
public:
    std::vector<std::unique_ptr<GameObject>> game_objects;
    std::unordered_map<std::type_index, std::unique_ptr<Service>> services;

    Scene() = default;
    virtual ~Scene() = default;

    void add_game_object(std::unique_ptr<GameObject> game_object) {
        game_object->scene = this;
        game_objects.push_back(std::move(game_object));
    }

    template <typename T, typename... TArgs>
    T* add_game_object(TArgs&&... args) {
        static_assert(std::is_base_of<GameObject, T>::value, "T must derive from GameObject");
        auto new_object = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* object_ptr = new_object.get();
        add_game_object(std::move(new_object));
        return object_ptr;
    }

    template <typename T>
    void add_service(std::unique_ptr<T> service) {
        service->scene = this;
        auto [it, inserted] = services.emplace(std::type_index(typeid(T)), std::move(service));
        if (!inserted) {
            TraceLog(LOG_ERROR, "Duplicate service added: %s", typeid(T).name());
        }
    }

    template <typename T, typename... TArgs>
    T* add_service(TArgs&&... args) {
        static_assert(std::is_base_of<Service, T>::value, "T must derive from Service");
        auto new_service = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* service_ptr = new_service.get();
        add_service<T>(std::move(new_service));
        return service_ptr;
    }

    template <typename T>
    T* get_service() {
        auto it = services.find(std::type_index(typeid(T)));
        if (it != services.end()) {
            auto service = it->second.get();
            if (!service->is_init) {
                TraceLog(LOG_ERROR, "Service not initialized: %s", typeid(T).name());
            }
            return static_cast<T*>(service);
        }
        TraceLog(LOG_FATAL, "Service of requested type not found in scene: %s", typeid(T).name());
        return nullptr;
    }

    std::vector<GameObject*> get_game_objects_with_tag(const std::string& tag) {
        std::vector<GameObject*> tagged_objects;
        for (auto& game_object : game_objects) {
            if (game_object->has_tag(tag)) {
                tagged_objects.push_back(game_object.get());
            }
        }
        return tagged_objects;
    }

    void init_services() {
        for (auto& service : services) {
            if (!service.second->is_init) {
                service.second->init();
            }
        }
    }

    virtual void init() {
        init_services();
        for (auto& game_object : game_objects) {
            game_object->init();
        }
    }

    virtual void load() {
        for (auto& game_object : game_objects) {
            game_object->load();
        }
    }

    virtual void update(float delta_time) {
        for (auto& game_object : game_objects) {
            game_object->update(delta_time);
        }
        for (auto& service : services) {
            service.second->update();
        }
    }

    virtual void draw() {
        for (auto& service : services) {
            service.second->draw();
        }
        for (auto& game_object : game_objects) {
            game_object->draw();
        }
    }
};

class PhysicsService : public Service {
public:
    b2WorldId world = b2_nullWorldId;
    b2Vec2 gravity = {0.0f, 10.0f};
    float time_step = 1.0f / 60.0f;
    int sub_steps = 6;
    float meters_to_pixels = 30.0f;
    float pixels_to_meters = 1.0f / meters_to_pixels;

    PhysicsService(b2Vec2 gravity = b2Vec2{0.0f, 10.0f}, float time_step = 1.0f / 60.0f, int sub_steps = 6, float meters_to_pixels = 30.0f) :
         gravity(gravity), time_step(time_step), sub_steps(sub_steps), meters_to_pixels(meters_to_pixels), pixels_to_meters(1.0f / meters_to_pixels) {}

    ~PhysicsService() {
        if (b2World_IsValid(world)) {
            b2DestroyWorld(world);
        }
    }

    void init() override {
        b2WorldDef world_def = b2DefaultWorldDef();
        world_def.gravity = gravity;
        world_def.contactHertz = 120;
        world = b2CreateWorld(&world_def);

        Service::init();
    }

    void update() override {
        if (!b2World_IsValid(world)) return;
        b2World_Step(world, time_step, sub_steps);
    }

    Vector2 convert_to_pixels(b2Vec2 meters) const {
        const auto converted = meters * meters_to_pixels;
        return {converted.x, converted.y};
    }

    b2Vec2 convert_to_meters(Vector2 pixels) const {
        return {pixels.x * pixels_to_meters, pixels.y * pixels_to_meters};
    }
};

struct IntVec2 { int x, y; };
static inline bool operator==(const IntVec2& a, const IntVec2& b){ return a.x==b.x && a.y==b.y; }

struct IntVec2Hash {
    size_t operator()(const IntVec2& p) const noexcept {
        std::size_t h1 = std::hash<int>{}(p.x);
        std::size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 1);
    }
};

// Undirected edge (a,b) stored canonically (min,max)
struct Edge {
    IntVec2 a, b;
};
static inline bool operator==(const Edge& e1, const Edge& e2){
    return e1.a == e2.a && e1.b == e2.b;
}
struct EdgeHash {
    size_t operator()(const Edge& e) const noexcept {
        IntVec2Hash h;
        std::size_t h1 = h(e.a);
        std::size_t h2 = h(e.b);
        return h1 ^ (h2 << 1);
    }
};

class LDtkService : public Service {
public:
    ldtk::Project project;
    std::string project_file;
    std::string level_name;
    std::vector<std::string> collision_names;
    std::vector<RenderTexture2D> renderers;
    std::vector<b2BodyId> layer_bodies;
    float scale = 4.0f;

    LDtkService(std::string project_file, std::string level_name, std::vector<std::string> collision_names, float scale = 4.0f) :
        project_file(project_file), level_name(level_name), collision_names(collision_names), scale(scale) {}

    virtual ~LDtkService() {
        for (auto& renderer : renderers) {
            UnloadRenderTexture(renderer);
        }

        for (auto& body : layer_bodies) {
            b2DestroyBody(body);
        }
    }

    void init() override {
        if (!FileExists(project_file.c_str())) {
            TraceLog(LOG_FATAL, "LDtk file not found: %s", project_file.c_str());
        }
        project.loadFromFile(project_file);
        const auto& world = project.getWorld();
        const auto& levels = world.allLevels();

        bool found = false;
        for (const auto& level : levels) {
            if (level.name == level_name) {
                found = true;
                break;
            }
        }
        if (!found) {
            TraceLog(LOG_FATAL, "LDtk level not found: %s", level_name.c_str());
        }

        const auto& level = world.getLevel(level_name);
        const auto& layers = level.allLayers();

        // Loop through all layers and create textures and collisions bodies.
        for (auto& layer : layers) {
            if (!layer.hasTileset()) {
                continue;
            }

            // Load the texture and the renderer.
            auto directory = std::string(GetDirectoryPath(project_file.c_str()));
            auto tileset_file = directory + "/" + layer.getTileset().path;
            if (!FileExists(tileset_file.c_str())) {
                TraceLog(LOG_FATAL, "Tileset file not found: %s", tileset_file.c_str());
            }
            Texture2D texture = LoadTexture(tileset_file.c_str());
            RenderTexture2D renderer = LoadRenderTexture(level.size.x, level.size.y);

            // Draw all the tiles.
            const auto& tiles_vector = layer.allTiles();
            BeginTextureMode(renderer);
            ClearBackground(MAGENTA);
            for (const auto &tile : tiles_vector) {
                const auto& position = tile.getPosition();
                const auto& texture_rect = tile.getTextureRect();
                Vector2 dest = {
                    static_cast<float>(position.x),
                    static_cast<float>(position.y),
                };
                Rectangle src = {
                    static_cast<float>(texture_rect.x),
                    static_cast<float>(texture_rect.y),
                    static_cast<float>(texture_rect.width) * (tile.flipX ? -1.0f : 1.0f),
                    static_cast<float>(texture_rect.height) * (tile.flipY ? -1.0f : 1.0f)
                };
                DrawTextureRec(texture, src, dest, WHITE);
            }
            EndTextureMode();

            UnloadTexture(texture);
            renderers.push_back(renderer);


            // Create bodies.
            const auto& size = layer.getGridSize();

            auto make_edge = [&](IntVec2 p0, IntVec2 p1) -> Edge {
                if (p1.x < p0.x || (p1.x == p0.x && p1.y < p0.y)) std::swap(p0, p1);
                return {p0, p1};
            };

            std::unordered_set<Edge, EdgeHash> edges;

            for (int y = 0; y < size.y; y++) {
                for (int x = 0; x < size.x; x++) {
                    if (!is_solid(layer, x, y, size)) continue;

                    // neighbor empty => boundary edge
                    if (!is_solid(layer, x, y - 1, size)) edges.insert(make_edge({x, y}, {x + 1, y}));
                    if (!is_solid(layer, x, y + 1, size)) edges.insert(make_edge({x, y + 1}, {x + 1, y + 1}));
                    if (!is_solid(layer, x - 1, y, size)) edges.insert(make_edge({x, y}, {x, y + 1}));
                    if (!is_solid(layer, x + 1, y, size)) edges.insert(make_edge({x + 1, y}, {x + 1, y + 1}));
                }
            }

            std::unordered_map<IntVec2, std::vector<IntVec2>, IntVec2Hash> adj;
            adj.reserve(edges.size() * 2);

            for (auto& e : edges) {
                adj[e.a].push_back(e.b);
                adj[e.b].push_back(e.a);
            }

            // Helper to remove an undirected edge from the set as we consume it
            auto erase_edge = [&](IntVec2 p0, IntVec2 p1){
                edges.erase(make_edge(p0, p1));
            };

            // Walk loops
            std::vector<std::vector<IntVec2>> loops;

            while (!edges.empty()) {
                // pick an arbitrary remaining edge
                Edge startE = *edges.begin();
                IntVec2 start = startE.a;
                IntVec2 cur = startE.b;
                IntVec2 prev = start;

                std::vector<IntVec2> poly;
                poly.push_back(start);
                poly.push_back(cur);
                erase_edge(start, cur);

                while (!(cur == start)) {
                    // choose next neighbor that is not prev and still has an edge remaining
                    const auto& nbs = adj[cur];
                    IntVec2 next = prev; // fallback

                    bool found = false;
                    for (const IntVec2& cand : nbs) {
                        if (cand == prev) continue;
                        if (edges.find(make_edge(cur, cand)) != edges.end()) {
                            next = cand;
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        // Open chain (should be rare for tile boundaries unless the boundary touches the map edge)
                        break;
                    }

                    prev = cur;
                    cur = next;
                    poly.push_back(cur);
                    erase_edge(prev, cur);

                    // safety guard to avoid infinite loops on bad topology
                    if (poly.size() > 100000) break;
                }

                // If closed, last vertex == start; Box2D chains usually want NOT duplicated end vertex.
                if (!poly.empty() && poly.back() == poly.front()) {
                    poly.pop_back();
                }

                // Only keep valid chains
                if (poly.size() >= 3) {
                    // If we're not solid on the right, then we wrapped the wrong way.
                    if (!loop_has_solid_on_right(poly, layer)) {
                        std::reverse(poly.begin(), poly.end());
                    }

                    // Not really necessary but here we reduce the number of points on a line to just the ends.
                    std::vector<IntVec2> reduced;
                    reduced.push_back(poly[0]);
                    b2Vec2 original_normal = {0, 0};
                    for (int i = 1; i < poly.size(); i++) {
                        auto first = poly[i - 1];
                        auto second = poly[i];
                        float length = sqrt((second.x - first.x) * (second.x - first.x) + (second.y - first.y) * (second.y - first.y));
                        b2Vec2 normal = { (second.x - first.x) / length, (second.y - first.y) / length };
                        if (length == 0) {
                            normal = {0, 0};
                        }
                        if (i == 1) {
                            original_normal = normal;
                        }

                        if (normal != original_normal) {
                            reduced.push_back(first);
                            original_normal = normal;
                        }

                    }
                    reduced.push_back(poly.back());
                    loops.push_back(std::move(reduced));
                }
            }

            auto physics = scene->get_service<PhysicsService>();

            b2BodyDef bd = b2DefaultBodyDef();
            bd.type = b2_staticBody;
            bd.position = {0,0};
            b2BodyId layer_body = b2CreateBody(physics->world, &bd);

            for (auto& loop : loops) {
                std::vector<b2Vec2> verts;
                verts.reserve(loop.size());

                for (auto& p : loop) {
                    float xpx = p.x * layer.getCellSize() * scale;
                    float ypx = p.y * layer.getCellSize() * scale;
                    verts.push_back({ xpx * physics->pixels_to_meters, ypx * physics->pixels_to_meters });
                }

                std::vector<b2SurfaceMaterial> mats;
                for (int i = 0; i < verts.size(); i++) {
                    b2SurfaceMaterial mat = b2DefaultSurfaceMaterial();
                    mat.friction = 0.1f;
                    mat.restitution = 0.1f;
                    mats.push_back(mat);
                }

                b2ChainDef cd = b2DefaultChainDef();
                cd.points = verts.data();
                cd.count  = (int)verts.size();
                cd.materials = mats.data();
                cd.materialCount = mats.size();
                cd.isLoop = true;
                b2CreateChain(layer_body, &cd);

                layer_bodies.push_back(layer_body);
            }
        }

        Service::init();
    }

    void draw() override {
        for (const auto& renderer : renderers) {
            Rectangle src = {
                0,
                0,
                static_cast<float>(renderer.texture.width),
                -static_cast<float>(renderer.texture.height)
            };
            Rectangle dest = {
                0,
                0,
                static_cast<float>(renderer.texture.width) * scale,
                static_cast<float>(renderer.texture.height) * scale
            };
            DrawTexturePro(renderer.texture, src, dest, {0}, .0f, WHITE);
        }
    }

    bool is_solid(const ldtk::Layer& layer, int x, int y, const ldtk::IntPoint& size) {
        if (x < 0 || y < 0 || x >= size.x || y >= size.y) return false;
        std::string name = layer.getIntGridVal(x, y).name;
        for (const auto& collision_name : collision_names) {
            if (name == collision_name) {
                return true;
            }
        }
        return false;
    };

    // Returns true if solid is on RIGHT side of the loop (correct for Box2D chain one-sided)
    bool loop_has_solid_on_right(const std::vector<IntVec2>& loop_corners, const ldtk::Layer& layer) {
        const int cell_size = layer.getCellSize();

        // pick an edge with non-zero length
        int n = (int)loop_corners.size();
        for (int i = 0; i < n; ++i) {
            IntVec2 a = loop_corners[i];
            IntVec2 b = loop_corners[(i + 1) % n];
            int dx = b.x - a.x;
            int dy = b.y - a.y;
            if (dx == 0 && dy == 0) continue;

            // Convert corner coords to pixel coords (scaled)
            float ax = a.x * cell_size * scale;
            float ay = a.y * cell_size * scale;
            float bx = b.x * cell_size * scale;
            float by = b.y * cell_size * scale;

            // edge direction
            float ex = bx - ax;
            float ey = by - ay;
            float len = std::sqrt(ex*ex + ey*ey);
            if (len < 1e-4f) continue;
            ex /= len; ey /= len;

            // right normal = (-ey, ex)
            float rx = -ey;
            float ry = ex;

            // midpoint of the edge
            float mx = 0.5f * (ax + bx);
            float my = 0.5f * (ay + by);

            // sample a point slightly to the right
            float eps = 0.25f * cell_size * scale; // quarter-tile in pixels (scaled)
            float sx = mx + rx * eps;
            float sy = my + ry * eps;

            // map sample pixel -> grid cell
            int gx = (int)std::floor(sx / (cell_size * scale));
            int gy = (int)std::floor(sy / (cell_size * scale));

            return is_solid(layer, gx, gy, layer.getGridSize());
        }

        // Fallback: if degenerate, say false
        return false;
    }

    /**
     * Get all entities across all layers in the level.
     *
     * @return A vector of LDtk entities.
     */
    std::vector<const ldtk::Entity*> get_entities() {
        const auto& world  = project.getWorld();
        const auto& level  = world.getLevel(level_name);
        const auto& layers = level.allLayers();

        std::vector<const ldtk::Entity*> entities;

        for (const auto& layer : layers) {
            const auto& layer_entities = layer.allEntities();

            entities.reserve(entities.size() + layer_entities.size());
            for (const auto& entity : layer_entities) {
                entities.push_back(&entity);
            }
        }

        return entities;
}

     std::vector<const ldtk::Entity*> get_entities_by_name(const std::string& name) {
        // TODO: Check if we loaded a project first.
        const auto& world = project.getWorld();
        const auto& level = world.getLevel(level_name);
        const auto& layers = level.allLayers();

        std::vector<const ldtk::Entity*> entities;

        for (const auto& layer : layers) {
            const auto& layer_entities = layer.getEntitiesByName(name);

            entities.reserve(entities.size() + layer_entities.size());
            for (const auto& entity : layer_entities) {
                entities.push_back(&entity.get());
            }
        }

        return entities;
    }

     std::vector<const ldtk::Entity*> get_entities_by_tag(const std::string& tag) {
        // TODO: Check if we loaded a project first.
        const auto& world = project.getWorld();
        const auto& level = world.getLevel(level_name);
        const auto& layers = level.allLayers();

        std::vector<const ldtk::Entity*> entities;

        for (const auto& layer : layers) {
            const auto& layer_entities = layer.getEntitiesByTag(tag);

            entities.reserve(entities.size() + layer_entities.size());
            for (const auto& entity : layer_entities) {
                entities.push_back(&entity.get());
            }
        }

        return entities;
    }

    const ldtk::Entity* get_entity_by_name(const std::string& name) {
        auto entities = get_entities_by_name(name);
        if (entities.empty()) {
            return nullptr;
        }

        return entities[0];
    }

    const ldtk::Entity* get_entity_by_tag(const std::string& tag) {
        auto entities = get_entities_by_tag(tag);
        if (entities.empty()) {
            return nullptr;
        }

        return entities[0];
    }

    Vector2 convert_to_pixels(const ldtk::IntPoint& point) const {
        return {point.x * scale, point.y * scale};
    }

    b2Vec2 convert_to_meters(const ldtk::IntPoint& point) const {
        auto physics = scene->get_service<PhysicsService>();
        physics->convert_to_meters(convert_to_pixels(point));
    }

    ldtk::IntPoint convert_to_grid(const Vector2& pixels) const {
        return {static_cast<int>(pixels.x / scale), static_cast<int>(pixels.y / scale)};
    }

    ldtk::IntPoint convert_to_grid(const b2Vec2& meters) const {
        auto physics = scene->get_service<PhysicsService>();
        auto pixels = physics->convert_to_pixels(meters);
        return {static_cast<int>(pixels.x / scale), static_cast<int>(pixels.y / scale)};
    }
};

class StaticBox : public GameObject {
public:
    b2BodyId body = b2_nullBodyId;
    float x, y, width, height;

    // TODO: Make the position the center of the the box like with the dynamic box.
    StaticBox(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}

    void init() override {
        auto physics = scene->get_service<PhysicsService>();
        const float pixels_to_meters = physics->pixels_to_meters;
        auto world = physics->world;

        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = b2_staticBody;
        body_def.position = b2Vec2{(x + width / 2.0f) * pixels_to_meters, (y + height / 2.0f) * pixels_to_meters};
        body = b2CreateBody(world, &body_def);

        b2Polygon body_polygon = b2MakeBox(width / 2.0f * pixels_to_meters, height / 2.0f * pixels_to_meters);
        b2ShapeDef box_shape_def = b2DefaultShapeDef();
        b2CreatePolygonShape(body, &box_shape_def, &body_polygon);
    }

    void draw() override {
        DrawRectangle(x, y, width, height, RED);
    }
};

class DynamicBox : public GameObject {
public:
    b2BodyId body = b2_nullBodyId;
    float x, y, width, height, rot_deg;

    DynamicBox(float x, float y, float width, float height, float rotation = 0)
        : x(x), y(y), width(width), height(height), rot_deg(rotation) {}

    DynamicBox(Vector2 position, Vector2 size, float rotation = 0)
        : x(position.x), y(position.y), width(size.x), height(size.y), rot_deg(rotation) {}

    void init() override {
        auto physics = scene->get_service<PhysicsService>();
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
    }

    void draw() override {
        auto physics = scene->get_service<PhysicsService>();
        float meters_to_pixels = physics->meters_to_pixels;
        b2Vec2 pos = b2Body_GetPosition(body);
        b2Rot rot = b2Body_GetRotation(body);
        float angle = b2Rot_GetAngle(rot) * RAD2DEG;

        DrawRectanglePro(
            {  pos.x * meters_to_pixels, pos.y * meters_to_pixels, width, height },
            { width / 2.0f, height / 2.0f },
            angle,
            RED
        );
    }
};

struct CharacterParams {
    // Geometry in pixels
    float width_px = 24.0f;
    float height_px = 40.0f;

    // Initial position in pixels
    Vector2 position;

    // Movement
    float max_speed_px_s = 220.0f;
    float accel_px_s2 = 2000.0f;
    float decel_px_s2 = 2500.0f;

    // Gravity / jump
    float gravity_px_s2 = 1400.0f;
    float jump_speed_px_s = 520.0f; // initial jump impulse (as velocity)
    float fall_speed_px_s = 1200.0f; // terminal velocity
    float jump_cut_mul = 0.45f; // when jump released early, meters_to_pixels upward vel

    // Forgiveness
    float coyote_time_s = 0.08f;
    float jump_buffer_s = 0.10f;

    // Raycast probes (in pixels)
    float ground_probe_px = 4.0f;
    float wall_probe_px = 3.0f;

    // Surface behavior
    float friction = 0.0f;
    float restitution = 0.0f;
    float density = 1.0f;
};

class Character : public GameObject {
public:
    b2BodyId body = b2_nullBodyId;

    CharacterParams p;

    bool grounded = false;
    bool on_wall_left = false;
    bool on_wall_right = false;
    float coyote_timer = 0.0f;
    float jump_buffer_timer = 0.0f;

    Character(CharacterParams p) : p(p) {}

    void init() override {
        auto physics = scene->get_service<PhysicsService>();
        const float pixels_to_meters = physics->pixels_to_meters;
        auto world = physics->world;

        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = b2_dynamicBody;
        body_def.fixedRotation = true;
        body_def.isBullet = true;
        body_def.linearDamping = 0.0f;
        body_def.angularDamping = 0.0f;
        body_def.position = physics->convert_to_meters(p.position);
        body = b2CreateBody(world, &body_def);

        b2SurfaceMaterial body_material = b2DefaultSurfaceMaterial();
        body_material.friction = p.friction;
        body_material.restitution = p.restitution;

        b2ShapeDef box_shape_def = b2DefaultShapeDef();
        box_shape_def.density = p.density;
        box_shape_def.material = body_material;

        // b2Polygon body_polygon = b2MakeBox(p.width_px / 2.0f * pixels_to_meters, p.height_px / 2.0f * pixels_to_meters);
        b2Polygon body_polygon = b2MakeRoundedBox(p.width_px / 2.0f * pixels_to_meters, p.height_px / 2.0f * pixels_to_meters, 1 * pixels_to_meters);
        b2CreatePolygonShape(body, &box_shape_def, &body_polygon);

        GameObject::init();
    }

    static float move_towards(float current, float target, float max_delta) {
        float delta = target - current;
        if (std::fabs(delta) <= max_delta) return target;
        return current + (delta > 0 ? max_delta : -max_delta);
    }

    void update(float delta_time) override {
        if (!b2Body_IsValid(body)) return;

        const float move_x = IsKeyDown(KEY_D) ? 1.0f : (IsKeyDown(KEY_A) ? -1.0f : 0.0f);
        const bool jump_pressed = IsKeyPressed(KEY_W);
        const bool jump_held = IsKeyDown(KEY_W);

        auto physics = scene->get_service<PhysicsService>();
        const float meters_to_pixels = physics->meters_to_pixels;
        const float pixels_to_meters = physics->pixels_to_meters;
        const b2WorldId world = physics->world;

        coyote_timer = std::max(0.0f, coyote_timer - delta_time);
        jump_buffer_timer = std::max(0.0f, jump_buffer_timer - delta_time);

        if (jump_pressed) {
            jump_buffer_timer = p.jump_buffer_s;
        }

        // Grounded check
        grounded = false;
        on_wall_left = false;
        on_wall_right = false;

        b2Vec2 pos = b2Body_GetPosition(body);
        b2Vec2 v_meters = b2Body_GetLinearVelocity(body);
        float vx = v_meters.x * meters_to_pixels;
        float vy = v_meters.y * meters_to_pixels;

        // Convert probe distances to meters
        float ground_probe = p.ground_probe_px * pixels_to_meters;
        float wall_probe = p.wall_probe_px * pixels_to_meters;

        float half_width = p.width_px / 2.0f * pixels_to_meters;
        float half_height = p.height_px / 2.0f * pixels_to_meters;
        // Ground: cast down from two points near the feet (left/right)
        b2Vec2 ground_left_start = { pos.x - half_width, pos.y + half_height};
        b2Vec2 ground_right_start = { pos.x + half_width, pos.y + half_height};
        b2Vec2 ground_translation = { 0, ground_probe };

        RayHit left_ground_hit = raycast_closest(world, body, ground_left_start, ground_translation);
        RayHit right_ground_hit = raycast_closest(world, body, ground_right_start, ground_translation);
        grounded = left_ground_hit.hit || right_ground_hit.hit;

        // Walls: cast left/right at mid-body height
        b2Vec2 mid = { pos.x, pos.y };
        b2Vec2 wall_left_start  = { pos.x - half_width, mid.y };
        b2Vec2 wall_left_translation = { -wall_probe, 0 };
        b2Vec2 wall_right_start = { pos.x + half_width, mid.y };
        b2Vec2 wall_right_translation = { wall_probe, 0 };

        RayHit left_wall_hit  = raycast_closest(world, body, wall_left_start, wall_left_translation);
        RayHit right_wall_hit = raycast_closest(world, body, wall_right_start, wall_right_translation);

        on_wall_left = left_wall_hit.hit;
        on_wall_right = right_wall_hit.hit;
        if (grounded) {
            coyote_timer = p.coyote_time_s;
        }

        float target_vx = move_x * p.max_speed_px_s;

        if (std::fabs(target_vx) > 0.001f) {
            float a = p.accel_px_s2;
            vx = move_towards(vx, target_vx, a * delta_time);
        } else {
            float a = p.decel_px_s2;
            vx = move_towards(vx, 0.0f, a * delta_time);
        }

        // Gravity (custom)
        vy += p.gravity_px_s2 * delta_time;
        vy = std::max(-p.fall_speed_px_s, std::min(p.fall_speed_px_s, vy));

        // Jump
        const bool can_jump = (grounded || coyote_timer > 0.0f);
        if (jump_buffer_timer > 0.0f && can_jump) {
            // Jump is negative Y if your screen coords are +down; adjust if needed.
            // Your gravity in earlier examples was +10 in Box2D meaning +down, so:
            vy = -p.jump_speed_px_s;
            jump_buffer_timer = 0.0f;
            coyote_timer = 0.0f;
            grounded = false;
        }

        // Variable jump height: cut upward velocity when jump released
        if (!jump_held && vy < 0.0f) {
            vy *= p.jump_cut_mul;
        }

        // Write velocity back in m/s
        b2Vec2 out_v = { vx * pixels_to_meters, vy * pixels_to_meters };
        b2Body_SetLinearVelocity(body, out_v);
    }

    void draw() override {
        auto physics = scene->get_service<PhysicsService>();
        float meters_to_pixels = physics->meters_to_pixels;
        float pixels_to_meters = physics->pixels_to_meters;

        Color color = grounded ? GREEN : BLUE;
        b2Vec2 pos = b2Body_GetPosition(body);
        DrawRectanglePro(
            {  pos.x * meters_to_pixels, pos.y * meters_to_pixels, p.width_px, p.height_px },
            { p.width_px / 2.0f, p.height_px / 2.0f },
            0.0f,
            color
        );
    }
};

class CameraObject : public GameObject {
public:
    Camera2D camera;

    void init() override {
        GameObject::init();
    }

    void draw_begin() {
        BeginMode2D(camera);
    }

    void draw_end() {
        EndMode2D();
    }
};

class Playground : public Scene {
public:
    DebugRenderer debug;

    void init() override {
        add_service<PhysicsService>();
        std::vector<std::string> collision_names = {"walls"};
        add_service<LDtkService>("assets/AutoLayers_1_basic.ldtk", "AutoLayer", collision_names);

        init_services();

        auto level = get_service<LDtkService>();
        auto player_entity = level->get_entity_by_name("Player");
        if (!player_entity) {
            assert(false);
        }
        auto box_entity = level->get_entity_by_tag("box");
        if (!box_entity) {
            assert(false);
        }

        CharacterParams params;
        params.position = level->convert_to_pixels(player_entity->getPosition());
        auto character = add_game_object<Character>(params);
        character->add_tag("character");

        auto position = level->convert_to_pixels(box_entity->getPosition());
        auto size = level->convert_to_pixels(box_entity->getSize());
        auto box = add_game_object<DynamicBox>(position, size, 46.0f);


        auto ground = add_game_object<StaticBox>(0.0f, 575.0f, 800.0f, 25.0f);
        ground->add_tag("ground");

        auto camera = add_game_object<CameraObject>();
        camera->add_tag("camera");
        camera->camera.zoom = 1;
        camera->camera.offset = {400, 300};
        camera->camera.rotation = 0;

        debug.init();

        Scene::init();
    }

    void update(float delta_time) override {
        auto camera = dynamic_cast<CameraObject*>(get_game_objects_with_tag("camera")[0]);
        auto player = dynamic_cast<Character*>(get_game_objects_with_tag("character")[0]);
        auto physics = get_service<PhysicsService>();
        camera->camera.target = physics->convert_to_pixels(b2Body_GetPosition(player->body));

        Scene::update(delta_time);
    }

    void draw() override {
        auto camera = dynamic_cast<CameraObject*>(get_game_objects_with_tag("camera")[0]);
        camera->draw_begin();
        Scene::draw();
        // debug.debug_draw(get_service<PhysicsService>()->world);
        camera->draw_end();
    }
};
