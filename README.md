# space-game

A simple space game built in C++ using [raylib](https://www.raylib.com/), implementing common game engine concepts.

Implementation includes:
- a simple entity component system (ECS)
- the Gilbert-Johnson-Keerthi algorithm (GJK), for collision detection
- the expanding polytope algorithm (EPA), for normal calculation in collisions
- Baumgarte positional correction

## Controls
- **`w`**: accelerate forwards
- **`Left Click`**: Accelerate rotation towards mouse
- **`c`**: toggle colliders
## Project Structure

```
space-game/
├── data/               # Game assets (textures, etc.)
├── game/
│   ├── src/            # Game source files (.cpp)
│   └── include/        # Game headers (.h)
├── libs/
│   └── game-core/      # Shared engine/core library
└── CMakeLists.txt
```

## External Dependencies

| Dependency | Version |
|------------|---------|
| [raylib](https://github.com/raysan5/raylib) | 5.5 |
| [tracy](https://github.com/wolfpld/tracy) | - |

No manual dependency installation is required, CMake will download external packages automatically at configure time using FetchContent.

## Building

> **Note:** Requires CMake 3.11+ and a C++17-compatible compiler.

### Linux

```bash
git clone https://github.com/elarner05/space-game.git
cd space-game/
mkdir build && cd build
cmake ..
make
./space-game
```

### Windows

```bash
git clone https://github.com/elarner05/space-game.git
cd space-game
mkdir build && cd build
cmake ..
cmake --build . --config Release
cd Release\                                            # needs to be in the release directory for relative paths
.\space-game.exe
```

> You'll need [Visual Studio](https://visualstudio.microsoft.com/) or [MinGW](https://www.mingw-w64.org/) installed, and CMake on your PATH.

### macOS

> macOS has not been tested. The steps below should work in theory but may require adjustments.

```bash
git clone https://github.com/elarner05/space-game.git
cd space-game
mkdir build && cd build
cmake ..
make
./space-game
```

You may need to install Xcode Command Line Tools:
```bash
xcode-select --install
```
