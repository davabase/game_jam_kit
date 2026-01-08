#pragma once

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <raylib.h>

// TODO: Write docs with principles. Use get_service, get_game_objects_*, and get_component in init and store references. This way if a service, object, or component does not exist it will cause a crash during init.
// TODO: Make WindowManager to store window size and init, etc. AudioManager for loading and playing sounds. SpriteManager for loading sprites.
// TODO: Move prebuilt Components, GameObjects, and Services to prefabs/ folder. Move ECS stuff and raycast helpers to utils/ folder.

// Forward declarations.
class GameObject;
class Scene;
class Game;

class Component {
public:
    GameObject* owner = nullptr;

    Component() = default;
    virtual ~Component() = default;

    virtual void init() {}
    virtual void update(float delta_time) {}
    virtual void draw() {}
};

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

    void update(float delta_time) {
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

class GameObject {
public:
    Scene* scene = nullptr;
    std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
    std::unordered_set<std::string> tags;

    GameObject() = default;
    virtual ~GameObject() = default;

    virtual void init() {
        for (auto& component : components) {
            component.second->init();
        }
    }

    virtual void update(float delta_time) {
        for (auto& component : components) {
            component.second->update(delta_time);
        }
    }

    virtual void draw() {
        for (auto& component : components) {
            component.second->draw();
        }
    }

    template <typename T>
    void add_component(std::unique_ptr<T> component) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        component->owner = this;
        auto [it, inserted] = components.emplace(std::type_index(typeid(T)), std::move(component));
        if (!inserted) {
            TraceLog(LOG_ERROR, "Duplicate component added: %s", typeid(T).name());
        }
    }

    template <typename T, typename... TArgs>
    T* add_component(TArgs&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        auto new_component = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* component_ptr = new_component.get();
        add_component<T>(std::move(new_component));
        return component_ptr;
    }

    template <typename T>
    T* get_component() {
        auto it = components.find(std::type_index(typeid(T)));
        if (it != components.end()) {
            auto component = it->second.get();
            return static_cast<T*>(component);
        }
        return nullptr;
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

template <typename T>
class MultiService : public Service {
public:
    std::unordered_map<std::string, std::unique_ptr<T>> services;

    MultiService() = default;
    void init() override {
        for (auto& service : services) {
            service.second->init();
        }
        Service::init();
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

class Manager {
public:
    bool is_init = false;

    Manager() = default;
    virtual ~Manager() = default;

    virtual void init() {
        is_init = true;
    }
};

template <typename T>
class MultiManager : public Manager {
public:
    std::unordered_map<std::string, std::unique_ptr<T>> managers;

    MultiManager() = default;

    void init() override {
        for (auto& manager : managers) {
            manager.second->init();
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

class Scene {
public:
    std::vector<std::unique_ptr<GameObject>> game_objects;
    std::unordered_map<std::type_index, std::unique_ptr<Service>> services;
    Game* game = nullptr;
    bool is_init = false;

    Scene() = default;
    virtual ~Scene() = default;

    virtual void on_init_services() {}
    virtual void on_init() {}
    virtual void on_update(float delta_time) {}
    virtual void on_draw() {}


    virtual void init() {
        if (is_init) {
            return;
        }

        on_init_services();

        for (auto& service : services) {
            if (!service.second->is_init) {
                service.second->init();
            }
        }

        on_init();

        for (auto& game_object : game_objects) {
            game_object->init();
        }
        is_init = true;
    }

    virtual void update(float delta_time) {
        on_update(delta_time);

        for (auto& game_object : game_objects) {
            game_object->update(delta_time);
        }
        for (auto& service : services) {
            service.second->update();
        }
    }

    virtual void draw() {
        on_draw();

        for (auto& service : services) {
            service.second->draw();
        }
        for (auto& game_object : game_objects) {
            game_object->draw();
        }
    }

    virtual void on_transition() {}

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
        static_assert(std::is_base_of<Service, T>::value, "T must derive from Service");
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

    template <typename T>
    T* get_manager() {
        if (!game) {
            TraceLog(LOG_FATAL, "No Game assigned to scene.");
        }
        return game->get_manager<T>();
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
};

class Game {
public:
    std::unordered_map<std::type_index, std::unique_ptr<Manager>> managers;
    std::map<std::string, std::unique_ptr<Scene>> scenes;
    Scene* current_scene = nullptr;
    Scene* next_scene = nullptr;

    Game() = default;
    ~Game() = default;

    void init() {
        for (auto& manager : managers) {
            if (!manager.second->is_init) {
                manager.second->init();
            }
        }
    }

    void update(float delta_time) {
        // TODO: When do we init scenes?
        current_scene->init();

        if (current_scene) {
            current_scene->update(delta_time);

            BeginDrawing();
            ClearBackground(RAYWHITE);

            current_scene->draw();

            EndDrawing();
        }

        // Switch scenes if needed.
        if (next_scene) {
            current_scene = next_scene;
            current_scene->on_transition();
            next_scene = nullptr;
        }
    }

    template <typename T>
    void add_manager(std::unique_ptr<T> manager) {
        static_assert(std::is_base_of<Manager, T>::value, "T must derive from Manager");
        auto [it, inserted] = managers.emplace(std::type_index(typeid(T)), std::move(manager));
        if (!inserted) {
            TraceLog(LOG_ERROR, "Duplicate manager added: %s", typeid(T).name());
        }
    }

    template <typename T, typename... TArgs>
    T* add_manager(TArgs&&... args) {
        static_assert(std::is_base_of<Manager, T>::value, "T must derive from Manager");
        auto new_manager = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* manager_ptr = new_manager.get();
        add_manager<T>(std::move(new_manager));
        return manager_ptr;
    }

    template <typename T>
    T* get_manager() {
        auto it = managers.find(std::type_index(typeid(T)));
        if (it != managers.end()) {
            auto manager = it->second.get();
            if (!manager->is_init) {
                TraceLog(LOG_ERROR, "Manager not initialized: %s", typeid(T).name());
            }
            return static_cast<T*>(manager);
        }
        TraceLog(LOG_FATAL, "Manager of requested type not found: %s", typeid(T).name());
        return nullptr;
    }

    void add_scene(std::string name, std::unique_ptr<Scene> scene) {
        scenes[name] = std::move(scene);
        scenes[name]->game = this;
        if (!current_scene) {
            current_scene = scenes[name].get();
        }
    }

    template <typename T, typename... TArgs>
    T* add_scene(std::string name, TArgs&&... args) {
        static_assert(std::is_base_of<Scene, T>::value, "T must derive from Scene");
        auto new_scene = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* scene_ptr = new_scene.get();
        add_scene(name, std::move(new_scene));
        if (!current_scene) {
            current_scene = scene_ptr;
        }
        return scene_ptr;
    }

    Scene* go_to_scene(const std::string& name) {
        auto it = scenes.find(name);
        if (it != scenes.end()) {
            next_scene = it->second.get();
        } else {
            TraceLog(LOG_ERROR, "Scene not found: %s", name.c_str());
        }
        return next_scene;
    }

    Scene* go_to_scene_next() {
        // Find the next scene in the map.
        if (current_scene) {
            auto it = scenes.begin();;
            while (it != scenes.end()) {
                if (it->second.get() == current_scene) {
                    ++it;
                    break;
                }
                ++it;
            }
            if (it != scenes.end()) {
                next_scene = it->second.get();
            } else {
                // Loop back to the first scene.
                next_scene = scenes.begin()->second.get();
            }
        }
        return next_scene;
    }

    Scene* go_to_scene_previous() {
        // Find the previous scene in the map.
        if (current_scene) {
            auto it = scenes.end();
            while (it != scenes.begin()) {
                --it;
                if (it->second.get() == current_scene) {
                    break;
                }
            }
            if (it != scenes.begin()) {
                next_scene = it->second.get();
            } else {
                // Loop back to the last scene.
                next_scene = std::prev(scenes.end())->second.get();
            }
        }
        return next_scene;
    }
};