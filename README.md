# Prerequesites

## CMake

**Download and install `CMake 3.24` or higher** from [https://cmake.org/download/](https://cmake.org/download/).

## VULKAN

**Download and install the `Vulkan SDK 1.3.216.0` or newer** from [https://vulkan.lunarg.com/](https://vulkan.lunarg.com/).

## Open Project

### In VS Code

1. Open the Framework as folder in VS Code.
2. Install the required extensions in VS Code (`ms-vscode.cpptools-extension-pack`, `slevesque.shader`). The first contains CMake Tools and CMake, as well as C++ extensions including clang-format. The latter is for shader code highlighting.
3. The contained `CMakeLists.txt` will automatically be used by Visual Studio to load the project and the CMake configuration process is started immediately and the CMake output should be visible automatically. If not, please find the CMake Tab on the leftmost bar in VS Code and find the button "Configure All Projects" on the top.
4. Select a kit for cmake on the bottom bar.
5. If you want to switch between `Debug` and `Release` mode, find the drop-down menu left of the kit dropdown, it should say `CMake: [Debug]: Ready` or `CMake: [Release]: Ready`.

## Installation commands

### Windows

```bash
cmake -S . -B build -A x64
cmake --build build --config Release
cmake --install build --config Release --prefix "${PWD}/build/install"
```

The game can then be found in `./build/install/`. Without the `--prefix`, the game will be in `YOUR DRIVE/Program Files/Swarm/`. However, the game needs admin rights to run there because of the needed write permissions for generated files.
