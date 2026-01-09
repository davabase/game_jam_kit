#pragma once

#include "engine/framework.h"

/**
 * For when you want multiple of the same manager.
 */
template <typename T>
class MultiManager : public Manager
{
public:
    std::unordered_map<std::string, std::unique_ptr<T>> managers;

    MultiManager() = default;

    /**
     * Initialize all managers.
     */
    void init() override
    {
        for (auto& manager : managers)
        {
            manager.second->init_manager();
        }
        Manager::init();
    }

    /**
     * Add a manager to the MultiManager.
     *
     * @param name The name to give the manager.
     * @param manager The manager to add.
     */
    void add_manager(std::string name, std::unique_ptr<T> manager)
    {
        static_assert(std::is_base_of<Manager, T>::value, "T must derive from Manager");
        managers[name] = std::move(manager);
    }

    /**
     * Create a manager and add it to the MultiManager.
     *
     * @param name The name to give the manager.
     * @param args The arguments to forward to the manager constructor.
     * @return A pointer to the added manager.
     */
    template <typename... TArgs>
    T* add_manager(std::string name, TArgs&&... args)
    {
        static_assert(std::is_base_of<Manager, T>::value, "T must derive from Manager");
        auto new_manager = std::make_unique<T>(std::forward<TArgs>(args)...);
        T* manager_ptr = new_manager.get();
        add_manager(name, std::move(new_manager));
        return manager_ptr;
    }

    /**
     * Get a manager by name.
     *
     * @param name The name of the manager.
     * @return A pointer to the manager.
     */
    T* get_manager(std::string name)
    {
        return managers[name].get();
    }
};

/**
 * Manager for handling fonts so they are not loaded multiple times.
 */
class FontManager : public Manager
{
public:
    std::unordered_map<std::string, Font> fonts;

    /**
     * Constructor for FontManager.
     * Loads the default font.
     */
    FontManager()
    {
        fonts["default"] = GetFontDefault();
    }

    ~FontManager()
    {
        for (auto& pair : fonts)
        {
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
    Font& load_font(const std::string& name, const std::string& filename, int size = 32)
    {
        if (fonts.find(name) != fonts.end())
        {
            return fonts[name];
        }

        Font font = LoadFontEx(filename.c_str(), size, nullptr, 0);
        fonts[name] = font;
        return fonts[name];
    }

    /**
     * Get a font by name.
     *
     * @param name The name of the font.
     * @return A reference to the font.
     */
    Font& get_font(const std::string& name)
    {
        return fonts[name];
    }

    /**
     * Set the texture filter for a font.
     *
     * @param name The name of the font.
     * @param filter The filter to set.
     */
    void set_texture_filter(const std::string& name, int filter)
    {
        if (fonts.find(name) != fonts.end())
        {
            SetTextureFilter(fonts[name].texture, filter);
        }
    }
};
