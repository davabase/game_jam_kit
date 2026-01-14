# Game Jam Kit

## Linux prerequisites
Install `xmake`, `cmake`, and opengl libs:
```bash
sudo apt-get install xmake
sudo apt-get install cmake
sudo apt-get install libgl1-mesa-dev
```

## Build
```bash
xmake
```

## Run
```bash
xmake run
```

## Switch to Debug
```bash
xmake config --mode debug
```
Then run with the VS Code debugger.

## Switch back to Release
```bash
xmake config --mode release
```
Then run with `xmake run`.

## Clean
```bash
xmake config --clean
```

## Generate include paths
This has to be every time a new file is added.
```bash
xmake project -k compile_commands
```

## Add compile_commands to settings
This has to be done to allow VS Code to find includes.
Add this line to `c_cpp_properties.json` by running the command `C/C++: Edit Configurations (JSON)` from the command pallette.
```json
"compileCommands": "${workspaceFolder}/compile_commands.json",
```