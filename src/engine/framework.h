#pragma once

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <raylib.h>

// Forward declarations.
class GameObject;
class Scene;
class Game;

/**
 * A simple object pool for reusing game objects.
 */
template <typename T>
class ObjectPool
{
public:
    std::vector<std::shared_ptr<T>> objects;

    ObjectPool() = default;

    /**
     * Constructor that preallocates a number of objects.
     *
     * @param initial_size The number of objects to preallocate.
     */
    ObjectPool(size_t initial_size)
    {
        objects.reserve(initial_size);
        for (size_t i = 0; i < initial_size; ++i)
        {
            objects.push_back(std::make_shared<T>());
        }
    }

    /**
     * Get an object from the pool. If no inactive objects are available, a new one is created.
     *
     * @return A shared pointer to the object.
     */
    std::shared_ptr<T> get_object()
    {
        for (auto& obj : objects)
        {
            if (!obj->is_active)
            {
                obj->is_active = true;
                return obj;
            }
        }
        // No inactive objects, create a new one.
        auto new_obj = std::make_shared<T>();
        new_obj->is_active = true;
        objects.push_back(new_obj);
        return new_obj;
    }

    void return_object(std::shared_ptr<T> obj)
    {
        obj->is_active = false;
    }
};

/**
 * The base class for all game object components.
 * Components are added to game objects to provide functionality.
 */
class Component
{
public:
    GameObject* owner = nullptr;

    Component() = default;
    virtual ~Component() = default;

    /**
     * Lifecycle function called when the component is initialized.
     */
    virtual void init() {}

    /**
     * Lifecycle function called every frame to update the component.
     *
     * @param delta_time The time elapsed since the last frame.
     */
    virtual void update(float delta_time) {}

    /**
     * Lifecycle function called every frame to draw the component.
     * Called within Raylib BeginDrawing()/EndDrawing() block.
     */
    virtual void draw() {}
};

/**
 * The base class for all game objects.
 * Game objects are the main entities in the game world.
 */
class GameObject
{
public:
    Scene* scene = nullptr;
    std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
    std::unordered_set<std::string> tags;
    bool is_active = true;

    GameObject() = default;
    virtual ~GameObject() = default;

    /**
     * Lifecycle function called when the game object is initialized.
     */
    virtual void init() {}

    /**
     * Lifecycle function called every frame to update the game object.
     * @param delta_time The time elapsed since the last frame.
     */
    virtual void update(float delta_time) {}

    /**
     * Lifecycle function called every frame to draw the game object.
     * Called within Raylib BeginDrawing()/EndDrawing() block.
     */
    virtual void draw() {}

    /**
     * Initialize the game object and its components.
     */
    virtual void init_object()
    {
        init();
        for (auto& component : components)
        {
            component.second->init();
        }
    }

    /**
     * Update the game object and its components.
     *
     * @param delta_time The time elapsed since the last frame.
     */
    virtual void update_object(float delta_time)
    {
        if (!is_active)
        {
            return;
        }
        update(delta_time);
        for (auto& component : components)
        {
            component.second->update(delta_time);
        }
    }

    /**
     * Draw the game object and its components.
     */
    virtual void draw_object()
    {
        if (!is_active)
        {
            return;
        }
        draw();
        for (auto& component : components)
        {
            component.second->draw();
        }
    }

    /**
     * Add a component to the game object.
     *
     * @param component The component to add.
     */
    template <typename T>
    void add_component(std::unique_ptr<T> component)
    {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        component->owner = this;
        auto [it, inserted] = components.emplace(std::type_index(typeid(T)), std::move(component));
        if (!inserted)
        {
            TraceLog(LOG_ERROR, "Duplicate component added: %s", typeid(T).name());
        }
    }

    /**
     * Add a component to the game object.
     *
     * @param args The arguments to forward to the component constructor.
     * @return A pointer to the added component.
     */
    template <typename T, typename... TArgs>
    T* add_component(TArgs&&... args)
    {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        auto new_component = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* component_ptr = new_component.get();
        add_component<T>(std::move(new_component));
        return component_ptr;
    }

    /**
     * Get a component of the specified type.
     *
     * @return A pointer to the component, or nullptr if not found.
     */
    template <typename T>
    T* get_component()
    {
        auto it = components.find(std::type_index(typeid(T)));
        if (it != components.end())
        {
            auto component = it->second.get();
            return static_cast<T*>(component);
        }
        return nullptr;
    }

    /**
     * Add a tag to the game object.
     *
     * @param tag The tag to add.
     */
    void add_tag(const std::string& tag)
    {
        tags.insert(tag);
    }

    /**
     * Remove a tag from the game object.
     *
     * @param tag The tag to remove.
     */
    void remove_tag(const std::string& tag)
    {
        tags.erase(tag);
    }

    /**
     * Check if the game object has the specified tag.
     *
     * @param tag The tag to check.
     * @return True if the game object has the tag, false otherwise.
     */
    bool has_tag(const std::string& tag) const
    {
        return tags.find(tag) != tags.end();
    }
};

/**
 * The base class for all services.
 * Services provide Scene level functionality and are accessible by all game objects in the scene.
 */
class Service
{
public:
    Scene* scene = nullptr;
    bool is_init = false;
    bool is_visible = true;

    Service() = default;
    virtual ~Service() = default;

    /**
     * Lifecycle function called when the service is initialized.
     */
    virtual void init() {}

    /**
     * Lifecycle function called every frame to update the service.
     *
     * @param delta_time The time elapsed since the last frame.
     */
    virtual void update(float delta_time) {}

    /**
     * Lifecycle function called every frame to draw the service.
     */
    virtual void draw() {}

    /**
     * Lifecycle function called every frame to draw the service.
     * Called within Raylib BeginDrawing()/EndDrawing() block.
     */
    virtual void draw_service() {
        if (is_visible)
        {
            draw();
        }
    }

    /**
     * Initialize the service.
     */
    virtual void init_service() final
    {
        if (is_init)
        {
            return;
        }
        init();
        is_init = true;
    }
};

/**
 * The base class for all managers.
 * Managers provide global functionality and are accessible by all scenes and game objects.
 */
class Manager
{
public:
    bool is_init = false;

    Manager() = default;
    virtual ~Manager() = default;

    /**
     * Lifecycle function called when the manager is initialized.
     */
    virtual void init() {}

    /**
     * Initialize the manager.
     */
    virtual void init_manager() final
    {
        if (is_init)
        {
            return;
        }
        init();
        is_init = true;
    }
};

/**
 * The base class for all scenes.
 * Scenes contain game objects and services, and manage the lifecycle for each.
 */
class Scene
{
public:
    std::vector<std::shared_ptr<GameObject>> game_objects;
    std::unordered_map<std::type_index, std::unique_ptr<Service>> services;
    Game* game = nullptr;
    bool is_init = false;

    Scene() = default;
    virtual ~Scene() = default;

    /**
     * Lifecycle function called to initialize services.
     * This happens before the scene's init() function so that services are ready for game objects.
     */
    virtual void init_services() {}

    /**
     * Lifecycle function called when the scene is initialized.
     */
    virtual void init() {}

    /**
     * Lifecycle function called every frame to update the scene.
     * @param delta_time The time elapsed since the last frame.
     */
    virtual void update(float delta_time) {}

    /**
     * Lifecycle function called every frame to draw the scene.
     * Called within Raylib BeginDrawing()/EndDrawing() block.
     */
    virtual void draw() {}

    /**
     * Initialize the scene, its services, and its game objects.
     * Can be overriden for custom initialization sequences.
     */
    virtual void init_scene()
    {
        if (is_init)
        {
            return;
        }
        init_services();

        for (auto& service : services)
        {
            service.second->init_service();
        }

        init();

        for (auto& game_object : game_objects)
        {
            game_object->init_object();
        }
        is_init = true;
    }

    /**
     * Update the scene, its services, and its game objects.
     * Can be overriden for custom update sequences.
     *
     * @param delta_time The time elapsed since the last frame.
     */
    virtual void update_scene(float delta_time)
    {
        update(delta_time);

        for (auto& service : services)
        {
            service.second->update(delta_time);
        }
        for (auto& game_object : game_objects)
        {
            game_object->update_object(delta_time);
        }
    }

    /**
     * Draw the scene, its services, and its game objects.
     * Can be overriden for custom draw sequences, especially when using cameras.
     */
    virtual void draw_scene()
    {
        draw();

        for (auto& service : services)
        {
            service.second->draw();
        }
        for (auto& game_object : game_objects)
        {
            game_object->draw_object();
        }
    }

    /**
     * Lifecycle function called when the scene is transitioned to.
     */
    virtual void on_enter() {}

    /**
     * Lifecycle function called when the scene is transitioned away from.
     */
    virtual void on_exit() {}

    /**
     * Add a game object to the scene.
     *
     * @param game_object The game object to add.
     */
    void add_game_object(std::shared_ptr<GameObject> game_object)
    {
        game_object->scene = this;
        game_objects.push_back(game_object);
    }

    /**
     * Create a game object and add it to the scene.
     *
     * @param args The arguments to forward to the game object constructor.
     * @return A pointer to the added game object.
     */
    template <typename T, typename... TArgs>
    std::shared_ptr<T> add_game_object(TArgs&&... args)
    {
        static_assert(std::is_base_of<GameObject, T>::value, "T must derive from GameObject");
        auto new_object = std::make_shared<T>(std::forward<TArgs>(args)...);
        add_game_object(new_object);
        return new_object;
    }

    /**
     * Add a service to the scene.
     *
     * @param service The service to add.
     */
    template <typename T>
    void add_service(std::unique_ptr<T> service)
    {
        static_assert(std::is_base_of<Service, T>::value, "T must derive from Service");
        service->scene = this;
        auto [it, inserted] = services.emplace(std::type_index(typeid(T)), std::move(service));
        if (!inserted)
        {
            TraceLog(LOG_ERROR, "Duplicate service added: %s", typeid(T).name());
        }
    }

    /**
     * Create a service and add it to the scene.
     *
     * @param args The arguments to forward to the service constructor.
     * @return A pointer to the added service.
     */
    template <typename T, typename... TArgs>
    T* add_service(TArgs&&... args)
    {
        static_assert(std::is_base_of<Service, T>::value, "T must derive from Service");
        auto new_service = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* service_ptr = new_service.get();
        add_service<T>(std::move(new_service));
        return service_ptr;
    }

    /**
     * Get a service of the specified type.
     *
     * @return A pointer to the service.
     */
    template <typename T>
    T* get_service()
    {
        auto it = services.find(std::type_index(typeid(T)));
        if (it != services.end())
        {
            auto service = it->second.get();
            if (!service->is_init)
            {
                TraceLog(LOG_ERROR, "Service not initialized: %s", typeid(T).name());
            }
            return static_cast<T*>(service);
        }
        TraceLog(LOG_FATAL, "Service of requested type not found in scene: %s", typeid(T).name());
        return nullptr;
    }

    /**
     * Get a manager of the specified type from the game.
     *
     * @return A pointer to the manager.
     */
    template <typename T>
    T* get_manager()
    {
        if (!game)
        {
            TraceLog(LOG_FATAL, "No Game assigned to scene.");
        }
        return game->get_manager<T>();
    }

    /**
     * Get all game objects with the specified tag.
     *
     * @param tag The tag to search for.
     * @return A vector of pointers to the game objects with the specified tag.
     */
    std::vector<GameObject*> get_game_objects_with_tag(const std::string& tag)
    {
        std::vector<GameObject*> tagged_objects;
        for (auto& game_object : game_objects)
        {
            if (game_object->has_tag(tag))
            {
                tagged_objects.push_back(game_object.get());
            }
        }
        return tagged_objects;
    }
};

/**
 * The main game class.
 * Manages scenes and global managers.
 */
class Game
{
public:
    std::unordered_map<std::type_index, std::unique_ptr<Manager>> managers;
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
    std::vector<std::string> scene_order;
    Scene* current_scene = nullptr;
    Scene* next_scene = nullptr;

    Game() = default;
    ~Game() = default;

    /**
     * Initialize all managers.
     */
    void init()
    {
        for (auto& manager : managers)
        {
            manager.second->init_manager();
        }
    }

    /**
     * Update the current scene.
     *
     * @param delta_time The time elapsed since the last frame.
     */
    void update(float delta_time)
    {
        if (current_scene)
        {
            // Scene is only initialized if it wasn't already.
            current_scene->init_scene();
            current_scene->update_scene(delta_time);

            BeginDrawing();
            ClearBackground(RAYWHITE);

            current_scene->draw_scene();

            EndDrawing();
        }

        // Switch scenes if needed.
        if (next_scene)
        {
            if (current_scene)
            {
                current_scene->on_exit();
            }
            current_scene = next_scene;
            current_scene->on_enter();
            next_scene = nullptr;
        }
    }

    /**
     * Add a manager to the game.
     *
     * @param manager The manager to add.
     */
    template <typename T>
    void add_manager(std::unique_ptr<T> manager)
    {
        static_assert(std::is_base_of<Manager, T>::value, "T must derive from Manager");
        auto [it, inserted] = managers.emplace(std::type_index(typeid(T)), std::move(manager));
        if (!inserted)
        {
            TraceLog(LOG_ERROR, "Duplicate manager added: %s", typeid(T).name());
        }
    }

    /**
     * Create a manager and add it to the game.
     *
     * @param args The arguments to forward to the manager constructor.
     * @return A pointer to the added manager.
     */
    template <typename T, typename... TArgs>
    T* add_manager(TArgs&&... args)
    {
        static_assert(std::is_base_of<Manager, T>::value, "T must derive from Manager");
        auto new_manager = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* manager_ptr = new_manager.get();
        add_manager<T>(std::move(new_manager));
        return manager_ptr;
    }

    /**
     * Get a manager of the specified type.
     *
     * @return A pointer to the manager.
     */
    template <typename T>
    T* get_manager()
    {
        auto it = managers.find(std::type_index(typeid(T)));
        if (it != managers.end())
        {
            auto manager = it->second.get();
            if (!manager->is_init)
            {
                TraceLog(LOG_ERROR, "Manager not initialized: %s", typeid(T).name());
            }
            return static_cast<T*>(manager);
        }
        TraceLog(LOG_FATAL, "Manager of requested type not found: %s", typeid(T).name());
        return nullptr;
    }

    /**
     * Add a scene to the game.
     *
     * @param name The name to give the scene.
     * @param scene The scene to add.
     */
    void add_scene(std::string name, std::unique_ptr<Scene> scene)
    {
        scenes[name] = std::move(scene);
        scenes[name]->game = this;
        scene_order.push_back(name);
        if (!current_scene)
        {
            current_scene = scenes[name].get();
        }
    }

    /**
     * Create a scene and add it to the game.
     *
     * @param name The name to give the scene.
     * @param args The arguments to forward to the scene constructor.
     * @return A pointer to the added scene.
     */
    template <typename T, typename... TArgs>
    T* add_scene(std::string name, TArgs&&... args)
    {
        static_assert(std::is_base_of<Scene, T>::value, "T must derive from Scene");
        auto new_scene = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* scene_ptr = new_scene.get();
        add_scene(name, std::move(new_scene));
        if (!current_scene)
        {
            current_scene = scene_ptr;
        }
        return scene_ptr;
    }

    /**
     * Transition to the specified scene by name.
     * The transition occurs at the end of the current update cycle.
     *
     * @param name The name of the scene to transition to.
     * @return A pointer to the next scene.
     */
    Scene* go_to_scene(const std::string& name)
    {
        auto it = scenes.find(name);
        if (it != scenes.end())
        {
            next_scene = it->second.get();
        }
        else
        {
            TraceLog(LOG_ERROR, "Scene not found: %s", name.c_str());
        }
        return next_scene;
    }

    /**
     * Transition to the next scene in the scene order.
     * Loops back to the first scene if at the end.
     *
     * @return A pointer to the next scene.
     */
    Scene* go_to_scene_next()
    {
        // Find the next scene.
        if (current_scene)
        {
            auto it = scenes.begin();
            while (it != scenes.end())
            {
                if (it->second.get() == current_scene)
                {
                    break;
                }
                ++it;
            }
            if (it != scenes.end())
            {
                std::string name = it->first;
                auto order_it = std::find(scene_order.begin(), scene_order.end(), name);
                if (order_it != scene_order.end() && std::next(order_it) != scene_order.end())
                {
                    std::string next_name = *(std::next(order_it));
                    next_scene = scenes[next_name].get();
                }
                else
                {
                    // Loop back to the first scene.
                    next_scene = scenes[scene_order[0]].get();
                }
            }
        }
        return next_scene;
    }

    /**
     * Transition to the previous scene in the scene order.
     * Loops back to the last scene if at the beginning.
     *
     * @return A pointer to the previous scene.
     */
    Scene* go_to_scene_previous()
    {
        // Find the previous scene.
        if (current_scene)
        {
            auto it = scenes.end();
            while (it != scenes.begin())
            {
                --it;
                if (it->second.get() == current_scene)
                {
                    break;
                }
            }
            if (it != scenes.begin())
            {
                std::string name = it->first;
                auto order_it = std::find(scene_order.begin(), scene_order.end(), name);
                if (order_it != scene_order.end() && order_it != scene_order.begin())
                {
                    std::string prev_name = *(std::prev(order_it));
                    next_scene = scenes[prev_name].get();
                }
                else
                {
                    // Loop back to the last scene.
                    next_scene = scenes[scene_order.back()].get();
                }
            }
        }
        return next_scene;
    }
};
