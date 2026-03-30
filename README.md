# game-versus-fight
Fight against a AI.

## Requirements

### Mandatory

**CMake** (>= 3.16)
- Build system (portable Windows/Linux/macOS)
- Install: https://cmake.org/download/
- Verify: `cmake --version`

**C Compiler** (C11 or later)
- GCC / Clang / MSVC
- On Linux: `sudo apt install build-essential` (GCC/Clang)
- On macOS: `xcode-select --install`
- On Windows: MSVC (Visual Studio) or MinGW
- Verify: `gcc --version` or `clang --version`


**SDL3** (>= 3.0)
- Rendering + input library
- Install: https://github.com/libsdl-org/SDL/releases


## Quick Start

After cloning, verify the toolchain:

```bash
cmake --version       # Should show >= 3.16
gcc --version         # or clang --version
```

Configure once, then build and run:

```bash
cmake --preset dev
cmake --build --preset run
```

This creates `build/`, compiles the project, and launches `game_app`.

## Build (Portable Linux/macOS/Windows)

Configuration (Debug):

```bash
cmake --preset dev
```

Build only:

```bash
cmake --build --preset build
```

Build and run:

```bash
cmake --build --preset run
```

Run tests:

```bash
ctest --preset test
```

Optional Make shortcuts (Linux/macOS or environments with `make`)