# Game Jam Kit
A simple framework for making small games with [raylib](https://www.raylib.com/), [box2d](https://box2d.org/), and [LDtk](https://ldtk.io/).

## Introduction
The framework in setup as a series of classes that manage the lifecycle of each other.

`Game` manages `Manager`s and `Scene`s.

A `Manager` holds resources that are used across scenes. The resources are loaded at `Game::init()`.

`Scene` manages `Service`s and `GameObject`s.

`Scene`s also perform game logic for each level.

A `Service` holds resources that are used in a single scene. The resources are loaded during init and disposed when the scene is disposed.

`GameObject` manages `Component`s

`GameObject`s also perform game logic for individual game entities.

A `Component` is a reusable tool for creating `GameObject` behavior.

Each of these pieces has lifecycle functions for `init()`, `update()`, and `draw()` that can be overridden when creating your own subclasses. These functions are called by the containing manager. If you do not wish for your class to be managed you shouldn't inherit from these base classes.

The managers also have larger overridable functions, `init_*()`, `update_*()`, and `draw_*()` that give you increased control over how the manager is used.

See `engine/prefabs` for prebuilt managers, services, game objects, and components.

See `samples` for examples on how to build a `Scene`.

See `main.cpp` for how to build a `Game`.

## Building
This project uses [xmake](https://xmake.io/) to build. Checkout `xmake.lua` for more details.

Using the [xmake extension](https://marketplace.visualstudio.com/items?itemName=tboox.xmake-vscode) for VS Code is highly recommended.

## Windows prerequisites
Install [Visual Studio Community Edition](https://visualstudio.microsoft.com/vs/community/), anything 2022 and over should be fine.
Make sure to install the C++ packages.
Install [xmake](https://xmake.io/guide/quick-start.html).


## Mac prerequisites
Install `xmake`:
```bash
brew install xmake
```

## Linux prerequisites
Install `xmake`, `cmake`, opengl libs, and x11 libs:
```bash
sudo apt-get install xmake
sudo apt-get install g++
sudo apt-get install cmake
sudo apt-get install libgl1-mesa-dev
sudo apt-get install xorg-dev libx11-dev libxfixes-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

## Build
```bash
xmake
```

## Run
```bash
xmake run
```

## Switch to debug mode
```bash
xmake config --mode debug
```
Then run with the VS Code debugger.

## Switch to release mode
```bash
xmake config --mode release
```
Then run with `xmake run`.

## Clean
```bash
xmake config --clean
```

## Setup header includes for your editor
### Option 1:
Generate compile_commands for VS Code.
```bash
xmake project -k compile_commands
```
This has to be every time a new file is added.

Add this line to `c_cpp_properties.json` by running the command `C/C++: Edit Configurations (JSON)` from the command pallette.
```json
"compileCommands": "${workspaceFolder}/compile_commands.json",
```

### Option 2:
Alternatively add the include directory for each library to your include path configs.
They are located here for Windows:
```
C:\Users\user\AppData\Local\.xmake\packages\r\raylib\5.5\xxxxx\include
C:\Users\user\AppData\Local\.xmake\packages\b\box2d\v3.1.1\xxxxx\include
C:\Users\user\AppData\Local\.xmake\packages\l\ldtkloader\1.5.3+1\xxxxx\include
```
and here for Linux:
```
~/.xmake/packages/r/raylib/5.5/xxxxx/include
~/.xmake/packages/b/box2d/v3.1.1/xxxxx/include
~/.xmake/packages/l/ldtkloader/1.5.3+1/xxxxx/include
```
The specific hashes are for the different platforms, targets, and modes, selecting any of them will give you what you need.
