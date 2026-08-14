cmake_minimum_required(VERSION 3.20)
project(MinecraftNative VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization Flags
if(MSVC)
    add_compile_options(/O2 /arch:armv8 /fp:fast /W3)
else()
    add_compile_options(-O3 -ffast-math -Wall -Wextra)
endif()

# Find SDL2
find_package(SDL2 REQUIRED PATHS /opt/homebrew/opt/sdl2 /opt/homebrew/opt/sdl2-compat /usr/local)
if(NOT SDL2_FOUND)
    find_path(SDL2_INCLUDE_DIR SDL2/SDL.h PATHS /opt/homebrew/include /opt/homebrew/opt/sdl2/include /usr/local/include)
    find_library(SDL2_LIBRARY NAMES SDL2 SDL2-2.0 PATHS /opt/homebrew/lib /opt/homebrew/opt/sdl2/lib /usr/local/lib)
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIR}
    /opt/homebrew/include
)

# Source files
set(SOURCES
    vendor/glad/glad.c
    src/main.cpp
    src/engine/Window.cpp
    src/engine/Shader.cpp
    src/engine/Camera.cpp
    src/world/Chunk.cpp
    src/world/ChunkMesher.cpp
    src/world/World.cpp
    src/entity/Player.cpp
    src/entity/Mob.cpp
    src/ui/Hud.cpp
    src/protocol/Connection.cpp
)

add_executable(MinecraftNative ${SOURCES})

# Target Link Libraries
if(TARGET SDL2::SDL2)
    target_link_libraries(MinecraftNative PRIVATE SDL2::SDL2 OpenGL::GL)
elseif(SDL2_LIBRARIES)
    target_link_libraries(MinecraftNative PRIVATE ${SDL2_LIBRARIES} OpenGL::GL)
elseif(SDL2_LIBRARY)
    target_link_libraries(MinecraftNative PRIVATE ${SDL2_LIBRARY} OpenGL::GL)
else()
    target_link_libraries(MinecraftNative PRIVATE SDL2 OpenGL::GL)
endif()

# Windows Sockets
if(WIN32)
    target_link_libraries(MinecraftNative PRIVATE ws2_32)
endif()

# Threads
find_package(Threads REQUIRED)
target_link_libraries(MinecraftNative PRIVATE Threads::Threads)
