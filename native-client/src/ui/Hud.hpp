#pragma once
#include <string>
#include <vector>
#include "../math/Math.hpp"
#include "../engine/Shader.hpp"
#include <glad/glad.h>

namespace mc {

class Hud {
public:
    Hud();
    ~Hud();

    void render(int screenWidth, int screenHeight, float health, int food, int selectedSlot);
    void addChatMessage(const std::string& message);

private:
    std::unique_ptr<Shader> uiShader;
    GLuint vao{0}, vbo{0};

    std::vector<std::string> chatHistory;

    void initUI();
    void drawRect(float x, float y, float w, float h, float r, float g, float b, float a);
};

} // namespace mc
