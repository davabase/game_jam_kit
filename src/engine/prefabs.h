#pragma once

#include <LDtkLoader/Project.hpp>

#include "framework.h"
#include "math_extensions.h"
#include "raycasts.h"

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

template <typename T>
class MultiService : public Service {
public:
    std::unordered_map<std::string, std::unique_ptr<T>> services;

    MultiService() = default;
    void init_service() override {
        for (auto& service : services) {
            service.second->init();
        }
        Service::init_service();
    }

    void update() override {
        for (auto& service : services) {
            service.second->update();
        }
        Service::update();
    }

    void draw() override {
        for (auto& service : services) {
            service.second->draw();
        }
        Service::draw();
    }

    void add_service(std::string name, std::unique_ptr<T> service) {
        static_assert(std::is_base_of<Service, T>::value, "T must derive from Service");
        services[name] = std::move(service);
    }

    template <typename... TArgs>
    T* add_service(std::string name, TArgs&&... args) {
        static_assert(std::is_base_of<Service, T>::value, "T must derive from Service");
        auto new_service = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* service_ptr = new_service.get();
        add_service(name, std::move(new_service));
        return service_ptr;
    }

    T* get_service(std::string name) {
        return services[name].get();
    }
};

template <typename T>
class MultiManager : public Manager {
public:
    std::unordered_map<std::string, std::unique_ptr<T>> managers;

    MultiManager() = default;

    void init() override {
        for (auto& manager : managers) {
            manager.second->init_manager();
        }
        Manager::init();
    }

    void add_manager(std::string name, std::unique_ptr<T> manager) {
        static_assert(std::is_base_of<Manager, T>::value, "T must derive from Manager");
        managers[name] = std::move(manager);
    }

    template <typename... TArgs>
    T* add_manager(std::string name, TArgs&&... args) {
        static_assert(std::is_base_of<Manager, T>::value, "T must derive from Manager");
        auto new_manager = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* manager_ptr = new_manager.get();
        add_manager(name, std::move(new_manager));
        return manager_ptr;
    }

    T* get_manager(std::string name) {
        return managers[name].get();
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
    }

    void update() override {
        if (!b2World_IsValid(world)) {
            return;
        }
        b2World_Step(world, time_step, sub_steps);
    }

    Vector2 convert_to_pixels(b2Vec2 meters) const {
        const auto converted = meters * meters_to_pixels;
        return {converted.x, converted.y};
    }

    b2Vec2 convert_to_meters(Vector2 pixels) const {
        const auto converted = pixels * pixels_to_meters;
        return {converted.x, converted.y};
    }

    float convert_to_pixels(float meters) const {
        return meters * meters_to_pixels;
    }

    float convert_to_meters(float pixels) const {
        return pixels * pixels_to_meters;
    }

    /**
     * Raycast in pixels.
     *
     * @param ignore Box2d body to ignore.
     * @param from The start point of the ray.
     * @param to The end point of the ray.
     *
     * @return A RayHit struct describing the hit.
     */
    RayHit raycast(b2BodyId ignore, Vector2 from, Vector2 to) {
        auto start = convert_to_meters(from);
        auto translation = convert_to_meters(to - from);

        return raycast_closest(world, ignore, start, translation);
    }

    std::vector<b2BodyId> circle_overlap(Vector2 center, float radius, b2BodyId ignore_body = b2_nullBodyId) {
        auto center_m = convert_to_meters(center);
        auto radius_m = convert_to_meters(radius);
        return circle_hit(world, ignore_body, center_m, radius_m);
    }

    std::vector<b2BodyId> rectangle_overlap(Rectangle rectangle, float rotation = 0.0f, b2BodyId ignore_body = b2_nullBodyId) {
        Vector2 size = {rectangle.width, rectangle.height};
        Vector2 center = {rectangle.x + size.x / 2.0f, rectangle.y + size.y / 2.0f};
        auto size_m = convert_to_meters(size);
        auto center_m = convert_to_meters(center);
        return rectangle_hit(world, ignore_body, center_m, size_m, rotation);
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

struct IntPointHash {
    size_t operator()(const ldtk::IntPoint& p) const noexcept {
        std::size_t h1 = std::hash<int>{}(p.x);
        std::size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 1);
    }
};

// Undirected edge (a,b) stored canonically (min,max)
struct Edge {
    ldtk::IntPoint a, b;
};
static inline bool operator==(const Edge& e1, const Edge& e2){
    return e1.a == e2.a && e1.b == e2.b;
}
struct EdgeHash {
    size_t operator()(const Edge& e) const noexcept {
        IntPointHash h;
        std::size_t h1 = h(e.a);
        std::size_t h2 = h(e.b);
        return h1 ^ (h2 << 1);
    }
};

class LevelService : public Service {
public:
    ldtk::Project project;
    std::string project_file;
    std::string level_name;
    std::vector<std::string> collision_names;
    std::vector<RenderTexture2D> renderers;
    std::vector<b2BodyId> layer_bodies;
    float scale = 4.0f;

    PhysicsService* physics;

    LevelService(std::string project_file, std::string level_name, std::vector<std::string> collision_names, float scale = 4.0f) :
        project_file(project_file), level_name(level_name), collision_names(collision_names), scale(scale) {}

    virtual ~LevelService() {
        for (auto& renderer : renderers) {
            UnloadRenderTexture(renderer);
        }

        for (auto& body : layer_bodies) {
            if (b2Body_IsValid(body)) {
                b2DestroyBody(body);
            }
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

        physics = scene->get_service<PhysicsService>();

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

            auto make_edge = [&](ldtk::IntPoint p0, ldtk::IntPoint p1) -> Edge {
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

            std::unordered_map<ldtk::IntPoint, std::vector<ldtk::IntPoint>, IntPointHash> adj;
            adj.reserve(edges.size() * 2);

            for (auto& e : edges) {
                adj[e.a].push_back(e.b);
                adj[e.b].push_back(e.a);
            }

            // Helper to remove an undirected edge from the set as we consume it
            auto erase_edge = [&](ldtk::IntPoint p0, ldtk::IntPoint p1){
                edges.erase(make_edge(p0, p1));
            };

            // Walk loops
            std::vector<std::vector<ldtk::IntPoint>> loops;

            while (!edges.empty()) {
                // pick an arbitrary remaining edge
                Edge startE = *edges.begin();
                ldtk::IntPoint start = startE.a;
                ldtk::IntPoint cur = startE.b;
                ldtk::IntPoint prev = start;

                std::vector<ldtk::IntPoint> poly;
                poly.push_back(start);
                poly.push_back(cur);
                erase_edge(start, cur);

                while (!(cur == start)) {
                    // choose next neighbor that is not prev and still has an edge remaining
                    const auto& nbs = adj[cur];
                    ldtk::IntPoint next = prev; // fallback

                    bool found = false;
                    for (const ldtk::IntPoint& cand : nbs) {
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
                    std::vector<ldtk::IntPoint> reduced;
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
                    verts.push_back(physics->convert_to_meters({xpx, ypx}));
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
    bool loop_has_solid_on_right(const std::vector<ldtk::IntPoint>& loop_corners, const ldtk::Layer& layer) {
        const int cell_size = layer.getCellSize();

        // pick an edge with non-zero length
        int n = (int)loop_corners.size();
        for (int i = 0; i < n; ++i) {
            ldtk::IntPoint a = loop_corners[i];
            ldtk::IntPoint b = loop_corners[(i + 1) % n];
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

    const ldtk::World& get_world() {
        return project.getWorld();
    }

    const ldtk::Level& get_level() {
        const auto& world  = project.getWorld();
        return world.getLevel(level_name);
    }

    /**
     * Get the level size in pixels.
     *
     * @return A Vector2 containing the size of the level.
     */
    Vector2 get_size() {
        const auto& level = get_level();
        return {level.size.x * scale, level.size.y * scale};
    }

    /**
     * Get all entities across all layers in the level.
     *
     * @return A vector of LDtk entities.
     */
    std::vector<const ldtk::Entity*> get_entities() {
        if (!is_init) {
            TraceLog(LOG_ERROR, "LDtk project not loaded.");
            return {};
        }
        const auto& level  = get_level();
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
        if (!is_init) {
            TraceLog(LOG_ERROR, "LDtk project not loaded.");
            return {};
        }
        const auto& level = get_level();
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
        if (!is_init) {
            TraceLog(LOG_ERROR, "LDtk project not loaded.");
            return {};
        }
        const auto& level = get_level();
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
        return physics->convert_to_meters(convert_to_pixels(point));
    }

    ldtk::IntPoint convert_to_grid(const Vector2& pixels) const {
        return {static_cast<int>(pixels.x / scale), static_cast<int>(pixels.y / scale)};
    }

    ldtk::IntPoint convert_to_grid(const b2Vec2& meters) const {
        auto pixels = physics->convert_to_pixels(meters);
        return {static_cast<int>(pixels.x / scale), static_cast<int>(pixels.y / scale)};
    }

    Vector2 get_entity_position(ldtk::Entity* entity) {
        return convert_to_pixels(entity->getPosition());
    }

    Vector2 get_entity_size(ldtk::Entity* entity) {
        return convert_to_pixels(entity->getSize());
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

    ~SpriteComponent() {
        UnloadTexture(sprite);
    }

    void init() override {
        sprite = LoadTexture(filename.c_str());
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

    Animation(const std::vector<std::string>& filenames, float fps = 15.0f, bool loop = true) : fps(fps), frame_timer(1.0f / fps), loop(loop) {
        for (const auto& filename : filenames) {
            frames.push_back(LoadTexture(filename.c_str()));
        }
    }

    ~Animation() {
        for (auto& frame : frames) {
            UnloadTexture(frame);
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
        auto new_animation = std::make_unique<Animation>(std::forward<TArgs>(args)...);
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

class StaticBox : public GameObject {
public:
    b2BodyId body = b2_nullBodyId;
    float x, y, width, height;

    StaticBox(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}

    void init() override {
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

    void draw() override {
        DrawRectangle(x - width / 2.0f, y - height / 2.0f, width, height, RED);
    }
};

class DynamicBox : public GameObject {
public:
    b2BodyId body = b2_nullBodyId;
    float x, y, width, height, rot_deg;
    PhysicsService* physics;

    DynamicBox(float x, float y, float width, float height, float rotation = 0)
        : x(x), y(y), width(width), height(height), rot_deg(rotation) {}

    DynamicBox(Vector2 position, Vector2 size, float rotation = 0)
        : x(position.x), y(position.y), width(size.x), height(size.y), rot_deg(rotation) {}

    void init() override {
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

    void draw() override {
        float meters_to_pixels = physics->meters_to_pixels;
        b2Vec2 pos = b2Body_GetPosition(body);
        b2Rot rot = b2Body_GetRotation(body);
        float angle = b2Rot_GetAngle(rot) * RAD2DEG;

        DrawRectanglePro(
            {  physics->convert_to_pixels(pos.x), physics->convert_to_pixels(pos.y), width, height },
            { width / 2.0f, height / 2.0f },
            angle,
            RED
        );
    }
};

class CameraObject : public GameObject {
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

    CameraObject(Vector2 size, Vector2 level_size = {0, 0}, Vector2 follow_speed = {1000, 1000}, float offset_left = 150, float offset_right = 150, float offset_top = 100, float offset_bottom = 100) :
        size(size),
        level_size(level_size),
        follow_speed(follow_speed),
        offset_left(offset_left),
        offset_right(offset_right),
        offset_top(offset_top),
        offset_bottom(offset_bottom) {}

    void init() override {
        camera.zoom = 1.0f;
        camera.offset = {size.x / 2.0f, size.y / 2.0f};
        camera.rotation = 0.0f;

        camera.target = target;
    }

    void update(float dt) override {
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
        if (dx < -dz_left_w) {
            desired.x = target.x + dz_left_w;
        } else if (dx > dz_right_w) {
            desired.x = target.x - dz_right_w;
        }

        if (dy < -dz_top_w) {
            desired.y = target.y + dz_top_w;
        } else if (dy > dz_bottom_w) {
            desired.y = target.y - dz_bottom_w;
        }

        // Apply tracking speed per axis.
        if (follow_speed.x < 0) {
            camera.target.x = desired.x;
        }
        else {
            camera.target.x = move_towards(camera.target.x, desired.x, follow_speed.x * dt);
        }

        if (follow_speed.y < 0) {
            camera.target.y = desired.y;
        }
        else {
            camera.target.y = move_towards(camera.target.y, desired.y, follow_speed.y * dt);
        }

        Vector2 half_view = {size.x / 2.0f * inv_zoom, size.y / 2.0f * inv_zoom};
        if (level_size.x > size.x) {
            camera.target.x = clamp(camera.target.x, half_view.x, level_size.x - half_view.x);
        }
        if (level_size.y > size.y) {
            camera.target.y = clamp(camera.target.y, half_view.y, level_size.y - half_view.y);
        }
    }

    float move_towards(float current, float target, float maxDelta) {
        float d = target - current;
        if (d >  maxDelta) return current + maxDelta;
        if (d < -maxDelta) return current - maxDelta;
        return target;
    }

    float clamp(float v, float lo, float hi) {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }

    void set_target(Vector2 target) {
        this->target = target;
    }

    void set_zoom(float zoom) {
        camera.zoom = zoom;
    }

    void set_rotation(float angle) {
        camera.rotation = angle;
    }

    void draw_begin() {
        BeginMode2D(camera);
    }

    void draw_end() {
        EndMode2D();
    }

    void debug_draw(Color c = {0, 255, 0, 120}) const {
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

    Vector2 screen_to_world(Vector2 point) {
        return GetScreenToWorld2D(point, camera);
    }
};

class SplitCamera : public CameraObject {
public:
    RenderTexture2D texture;

    SplitCamera(Vector2 size, Vector2 level_size = {0, 0}, Vector2 follow_speed = {1000, 1000}, float offset_left = 150, float offset_right = 150, float offset_top = 100, float offset_bottom = 100) :
        CameraObject(size, level_size, follow_speed, offset_left, offset_right, offset_top, offset_bottom) {}

    ~SplitCamera() {
        UnloadRenderTexture(texture);
    }

    void init() override {
        texture = LoadRenderTexture(size.x, size.y);
        CameraObject::init();
    }

    void draw_begin() {
        BeginTextureMode(texture);
        ClearBackground(WHITE);
        BeginMode2D(camera);
    }

    void draw_end() {
        EndMode2D();
        EndTextureMode();
    }

    void draw_texture(float x, float y) {
        DrawTextureRec(texture.texture, {0, 0, static_cast<float>(texture.texture.width), static_cast<float>(-texture.texture.height)}, {x, y}, WHITE);
    }

    Vector2 screen_to_world(Vector2 draw_position, Vector2 point) {
        auto local_point = point - draw_position;
        return GetScreenToWorld2D(local_point, camera);
    }
};

class FontManager : public Manager {
public:
    std::unordered_map<std::string, Font> fonts;

    FontManager() {
        fonts["default"] = GetFontDefault();
    }

    ~FontManager() {
        for (auto& pair : fonts) {
            UnloadFont(pair.second);
        }
    }

    /**
     * Load a font from a file.
     *
     * @param name The name to associate with the font.
     * @param filename The filename of the font to load.
     * @param size The font size to save the font texture as.
     *
     * @return A reference to the loaded font.
     */
    Font& load_font(const std::string& name, const std::string& filename, int size = 32) {
        if (fonts.find(name) != fonts.end()) {
            return fonts[name];
        }

        Font font = LoadFontEx(filename.c_str(), size, nullptr, 0);
        fonts[name] = font;
        return fonts[name];
    }

    Font& get_font(const std::string& name) {
        return fonts[name];
    }

    void set_texture_filter(const std::string& name, int filter) {
        if (fonts.find(name) != fonts.end()) {
            SetTextureFilter(fonts[name].texture, filter);
        }
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

    TextComponent(FontManager* font_manager, std::string text, std::string font_name = "default", int font_size = 20, Color color = WHITE)
        : font_manager(font_manager), text(text), font_name(font_name), font_size(font_size), color(color) {}

    void draw() override {
        DrawTextEx(font_manager->get_font(font_name), text.c_str(), position, static_cast<float>(font_size), 1.0f, color);
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

struct CharacterParams {
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

class Character : public GameObject {
public:
    CharacterParams p;
    PhysicsService* physics;
    BodyComponent* body;
    MovementComponent* movement;
    AnimationController* animation;
    TextComponent* text_component;

    bool grounded = false;
    bool on_wall_left = false;
    bool on_wall_right = false;
    float coyote_timer = 0.0f;
    float jump_buffer_timer = 0.0f;

    Character(CharacterParams p) : p(p) {}

    void init() override {
        physics = scene->get_service<PhysicsService>();

        body = add_component<BodyComponent>([=](BodyComponent& b){
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

            b2Polygon body_polygon = b2MakeRoundedBox(physics->convert_to_meters(p.width / 2.0f), physics->convert_to_meters(p.height / 2.0f), physics->convert_to_meters(0.25));
            b2CreatePolygonShape(b.id, &box_shape_def, &body_polygon);
        });

        MovementParams mp;
        mp.width = p.width;
        mp.height = p.height;
        movement = add_component<MovementComponent>(mp);

        animation = add_component<AnimationController>(body);
        animation->add_animation("idle", std::vector<std::string>{"assets/character_green_idle.png"}, 1.0f);
        animation->add_animation("walk", std::vector<std::string>{"assets/character_green_walk_a.png", "assets/character_green_walk_b.png"}, 5.0f);
        animation->play("walk");
        animation->set_scale(0.35f);

        text_component = add_component<TextComponent>(scene->get_manager<FontManager>(), "Character", "Roboto", 128, WHITE);
        text_component->position = {p.position.x, p.position.y - p.height / 2.0f - 20.0f};
    }

    void update(float delta_time) override {
        int gamepad = 0;
        float deadzone = 0.1f;

        const bool jump_pressed = IsKeyPressed(KEY_W) || IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        const bool jump_held = IsKeyDown(KEY_W) || IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

        float move_x = 0.0f;
        move_x = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
        if (fabsf(move_x) < deadzone) {
            move_x = 0.0f;
        }
        if (IsKeyDown(KEY_D) || IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
            move_x = 1.0f;
        }
        else if (IsKeyDown(KEY_A) || IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
            move_x = -1.0f;
        }

        if (move_x < 0.0f) {
            animation->set_flip_x(true);
        } else if (move_x > 0.0f) {
            animation->set_flip_x(false);
        }
        if (fabsf(move_x) > 0.1f) {
            animation->play("walk");
        }
        else {
            animation->play("idle");
        }

        movement->set_input(move_x, jump_pressed, jump_held);
    }

    void draw() override {
        Color color = movement->grounded ? GREEN : BLUE;
        auto pos = body->get_position_pixels();
        DrawRectanglePro(
            {  pos.x, pos.y, p.width, p.height },
            { p.width / 2.0f, p.height / 2.0f },
            0.0f,
            color
        );
    }
};
