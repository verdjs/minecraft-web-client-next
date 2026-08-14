# Minecraft Native Client (C++20 ARM64 / Native)

A high-performance, native C++20 Minecraft client built without Electron or Web runtime overhead.

## Architecture

- **Dedicated Multi-Core CPU Meshing**: Greedy and culled chunk meshing running asynchronously across background CPU worker threads in a thread pool (`ThreadPool.hpp`).
- **Hardware-Accelerated GPU Rendering**: Direct OpenGL 3.3 Core shader pipeline with VAO/VBO vertex streams.
- **Voxel Physics & Smooth Controls**: Complete voxel AABB collision detection, gravity, friction, jumping, sneaking, and sprinting.
- **First-Person Item Bobbing & Eating Animation**: Smooth mouth translation and rhythmic chewing oscillation.
- **Living Mob Animations**: Biped and quadruped leg/arm walking oscillation cycles based on delta distance.
- **Native 2D HUD**: Crosshair, 9-slot hotbar with selection indicators, health hearts, and hunger meter.
- **Asynchronous Protocol Engine**: Non-blocking TCP client supporting Minecraft 1.20–1.21.x VarInt packet framing and compression.

---

## Building on Windows ARM64

### Requirements
- **Visual Studio 2022** with Desktop C++ & ARM64 build tools (or LLVM/Clang for ARM64)
- **CMake 3.20+**
- **SDL2** development libraries for Windows ARM64

### Compilation
```cmd
mkdir build
cd build
cmake -A ARM64 ..
cmake --build . --config Release
```

The resulting `MinecraftNative.exe` runs natively on Windows on ARM with full hardware acceleration.

---

## Building on macOS / Linux

### Requirements
- `cmake`
- `sdl2` (`brew install sdl2` on macOS or `sudo apt install libsdl2-dev` on Linux)
- C++20 compiler (`clang++` or `g++`)

### Compilation
```bash
mkdir build
cd build
cmake ..
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
./MinecraftNative
```
