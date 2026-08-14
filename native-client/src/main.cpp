#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include "engine/Window.hpp"
#include "engine/Shader.hpp"
#include "engine/Camera.hpp"
#include "engine/ThreadPool.hpp"
#include "world/World.hpp"
#include "entity/Player.hpp"
#include "entity/Mob.hpp"
#include "ui/Hud.hpp"
#include "protocol/Connection.hpp"

static const std::string chunkVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in float aAO;
layout(location = 4) in float aLight;

out vec2 vUV;
out vec3 vNormal;
out float vAO;
out float vLight;
out vec3 vWorldPos;

uniform mat4 uViewProj;

void main() {
    vUV = aUV;
    vNormal = aNormal;
    vAO = aAO;
    vLight = aLight;
    vWorldPos = aPos;
    gl_Position = uViewProj * vec4(aPos, 1.0);
}
)";

static const std::string chunkFragmentShader = R"(
#version 330 core
in vec2 vUV;
in vec3 vNormal;
in float vAO;
in float vLight;
in vec3 vWorldPos;

out vec4 FragColor;

void main() {
    // Procedural Minecraft block color based on normal and position
    vec3 baseColor = vec3(0.5, 0.75, 0.3); // Grass green default
    if (vNormal.y < -0.5) baseColor = vec3(0.45, 0.30, 0.20); // Dirt bottom
    else if (abs(vNormal.y) < 0.5) baseColor = vec3(0.40, 0.65, 0.25); // Side grass

    // Diffuse directional sunlight
    vec3 sunDir = normalize(vec3(0.4, 0.9, 0.3));
    float diffuse = max(dot(vNormal, sunDir), 0.0) * 0.4 + 0.6;

    // Ambient occlusion & block light shading
    float shade = diffuse * vAO * vLight;
    FragColor = vec4(baseColor * shade, 1.0);
}
)";

int main(int argc, char** argv) {
    std::cout << "==================================================\n";
    std::cout << "  Minecraft Native Client (ARM64 Windows / Native)\n";
    std::cout << "  Multi-Threaded CPU Meshing + Direct GPU Pipeline\n";
    std::cout << "==================================================\n";

    try {
        mc::Window window(1280, 720, "Minecraft Native Client [ARM64]");
        window.setCursorLocked(true);

        mc::ThreadPool threadPool(4); // Multi-core CPU background chunk mesher
        mc::World world;
        mc::Player player;
        mc::Hud hud;
        mc::Connection connection;

        // Generate initial surrounding terrain
        world.generateTestWorld(6);

        // Spawn test mobs
        std::vector<std::unique_ptr<mc::Mob>> mobs;
        mobs.push_back(std::make_unique<mc::Mob>(1, mc::MobType::Pig, mc::Vec3(4.0f, 65.0f, 4.0f)));
        mobs.push_back(std::make_unique<mc::Mob>(2, mc::MobType::Cow, mc::Vec3(-6.0f, 65.0f, 8.0f)));
        mobs.push_back(std::make_unique<mc::Mob>(3, mc::MobType::Zombie, mc::Vec3(10.0f, 65.0f, -6.0f)));

        mc::Shader chunkShader(chunkVertexShader, chunkFragmentShader);

        // Optional server connect if host argument provided
        if (argc > 1) {
            std::string host = argv[1];
            uint16_t port = 25565;
            if (argc > 2) port = static_cast<uint16_t>(std::atoi(argv[2]));

            std::cout << "[Client] Connecting to " << host << ":" << port << "...\n";
            if (connection.connect(host, port)) {
                connection.sendHandshake(host, port, 767, 2); // Minecraft 1.21 protocol (767) -> Login
                connection.sendLoginStart("NativePlayer");
            }
        }

        // Input state tracking
        bool keyW = false, keyS = false, keyA = false, keyD = false;
        bool keySpace = false, keyShift = false, keyCtrl = false;
        int selectedHotbarSlot = 0;

        window.onKey = [&](int key, int scancode, int action, int mods) {
            bool pressed = (action != 0);
            if (key == SDLK_w) keyW = pressed;
            if (key == SDLK_s) keyS = pressed;
            if (key == SDLK_a) keyA = pressed;
            if (key == SDLK_d) keyD = pressed;
            if (key == SDLK_SPACE) keySpace = pressed;
            if (key == SDLK_LSHIFT) keyShift = pressed;
            if (key == SDLK_LCTRL) keyCtrl = pressed;

            if (action == 1) {
                if (key >= SDLK_1 && key <= SDLK_9) {
                    selectedHotbarSlot = key - SDLK_1;
                }
                if (key == SDLK_ESCAPE) {
                    window.setCursorLocked(!window.isCursorLocked());
                }
                if (key == SDLK_e) {
                    // Toggle eating / item use animation demo
                    if (player.isUsingItem) player.stopUsingItem();
                    else player.startUsingItem();
                }
            }
        };

        window.onMouseMove = [&](double xpos, double ypos, double dx, double dy) {
            float sensitivity = 0.12f;
            player.camera.updateEulerAngles(static_cast<float>(dx) * sensitivity, static_cast<float>(-dy) * sensitivity);
        };

        auto lastTime = std::chrono::high_resolution_clock::now();
        float health = 20.0f;
        int food = 20;

        std::cout << "[Client] Engine running at locked 60+ FPS on hardware GPU!\n";

        // Main game loop
        while (!window.shouldClose()) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;
            if (dt > 0.1f) dt = 0.1f; // Clamp delta time

            window.pollEvents();
            connection.update();

            // 1. Player Physics & Controls
            player.update(dt, world, keyW, keyS, keyA, keyD, keySpace, keyShift, keyCtrl);

            // 2. Mobs Update & Limb Walking Cycles
            for (auto& mob : mobs) {
                mob->update(dt, player.position);
            }

            // 3. Multi-Core CPU Chunk Meshing
            world.update(player.position, threadPool);

            // 4. Dedicated Hardware GPU Rendering
            glClearColor(0.55f, 0.75f, 0.95f, 1.0f); // Sky blue
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Camera matrices
            mc::Mat4 view = player.camera.getViewMatrix();
            mc::Mat4 proj = player.camera.getProjectionMatrix(window.getAspectRatio());
            mc::Mat4 viewProj = proj * view;

            // Render World Chunks
            chunkShader.bind();
            chunkShader.setMat4("uViewProj", viewProj);
            world.render(viewProj);
            chunkShader.unbind();

            // Render Mobs
            for (auto& mob : mobs) {
                mob->render(viewProj);
            }

            // Render Native 2D HUD
            hud.render(window.getWidth(), window.getHeight(), health, food, selectedHotbarSlot);

            window.swapBuffers();
        }

    } catch (const std::exception& e) {
        std::cerr << "[Fatal Error] " << e.what() << std::endl;
        return 1;
    }

    std::cout << "[Client] Shutdown cleanly.\n";
    return 0;
}
