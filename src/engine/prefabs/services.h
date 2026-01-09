#pragma once

#include <LDtkLoader/Project.hpp>
#include "engine/framework.h"
#include "engine/physics_debug.h"

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

class TextureService : public Service {
public:
    std::unordered_map<std::string, Texture2D> textures;

    TextureService() = default;
    ~TextureService() {
        for (auto& pair : textures) {
            UnloadTexture(pair.second);
        }
    }

    Texture2D& get_texture(const std::string& filename) {
        if (textures.find(filename) == textures.end()) {
            Texture2D texture = LoadTexture(filename.c_str());
            textures[filename] = texture;
        }
        return textures[filename];
    }
};

class SoundService : public Service {
public:
    std::unordered_map<std::string, std::vector<Sound>> sounds;

    SoundService() = default;
    ~SoundService() {
        for (auto& pair : sounds) {
            for (auto& sound : pair.second) {
                UnloadSound(sound);
            }
        }
    }

    Sound& get_sound(const std::string& filename) {
        if (sounds.find(filename) == sounds.end()) {
            Sound sound = LoadSound(filename.c_str());
            sounds[filename] = { sound };
        } else {
            // Create a new alias to allow overlapping sounds.
            Sound sound = LoadSoundAlias(sounds[filename][0]);
            sounds[filename].push_back(sound);
        }
        return sounds[filename].back();
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
    PhysicsDebugRenderer debug_draw;

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
        debug_draw.init(meters_to_pixels);
    }

    void update() override {
        if (!b2World_IsValid(world)) {
            return;
        }
        b2World_Step(world, time_step, sub_steps);
    }

    void draw_debug() {
        debug_draw.debug_draw(world);
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
            auto texture_service = scene->get_service<TextureService>();
            Texture2D texture = texture_service->get_texture(tileset_file);
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
