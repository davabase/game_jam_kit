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

// TODO: Write docs with principles. Use get_service, get_game_objects_*, and get_component in init and store references. This way if a service, object, or component does not exist it will cause a crash during init.
// TODO: Make Manager class, like a global Service that survives across Scenes. Make Scene::init() take map of managers.
// TODO: Make WindowManager to store window size and init, etc. AudioManager for loading and playing sounds. SpriteManager for loading sprites.
// TODO: Move prebuilt Components, GameObjects, and Services to prefabs/ folder. Move ECS stuff and raycast helpers to utils/ folder.

// Forward declarations.
class GameObject;
class Scene;

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
    MultiComponent(std::unordered_map<std::string, std::unique_ptr<T>> components) : components(components) {}

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
        components[name] = std::move(component);
    }

    template <typename... TArgs>
    T* add_component(std::string name, TArgs&&... args) {
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
