add_rules("mode.debug", "mode.release")

set_languages("c++17")

if is_plat("wasm") then
    set_toolchains("emcc")
end

-- Packages
add_requires("raylib", "box2d", "ldtkloader")

target("game_jam_kit")
    set_kind("binary")
    add_files("src/*.cpp")
    add_includedirs("src", { public = true })
    add_packages("raylib", "box2d", "ldtkloader")

    -- Copy assets to output directory after build
    after_build(function (target)
        local outdir = target:targetdir()
        os.mkdir(outdir)
        os.cp("assets", outdir)
    end)

    -- Emscripten / WebAssembly specific settings
    if is_plat("wasm") then
        add_cxxflags("-O3", {force = true})
        add_ldflags(
            "-O3",
            -- For in-browser wasm debugging.
            -- "-g",
            "-s USE_GLFW=3",
            "-s ASYNCIFY",
            "-s ALLOW_MEMORY_GROWTH=1",
            "-s EXPORT_NAME=Module",
            "-s EXPORTED_RUNTIME_METHODS=[ccall,cwrap]",
            "-s FORCE_FILESYSTEM=1",
            "-s ENVIRONMENT=web",
            "-s WASM=1",
            "-lwebsocket.js",
            -- "--shell-file=web/shell_minimal.html",
            -- Add files to the virtual file system.
            "--preload-file", "assets@/assets",
            {force = true}
        )
        set_extension(".html")
    end
