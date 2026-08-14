#include "Hud.hpp"

namespace mc {

static const std::string uiVertexShader = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;

out vec4 vColor;
uniform mat4 uProjection;

void main() {
    vColor = aColor;
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
}
)";

static const std::string uiFragmentShader = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main() {
    FragColor = vColor;
}
)";

Hud::Hud() {
    uiShader = std::make_unique<Shader>(uiVertexShader, uiFragmentShader);
    initUI();
}

Hud::~Hud() {
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
}

void Hud::initUI() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void Hud::drawRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    float vertices[] = {
        x,     y,     r, g, b, a,
        x + w, y,     r, g, b, a,
        x + w, y + h, r, g, b, a,
        x + w, y + h, r, g, b, a,
        x,     y + h, r, g, b, a,
        x,     y,     r, g, b, a,
    };

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Hud::render(int screenWidth, int screenHeight, float health, int food, int selectedSlot) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    uiShader->bind();

    // Orthographic projection matrix
    Mat4 ortho{};
    ortho.m[0] = 2.0f / screenWidth;
    ortho.m[5] = -2.0f / screenHeight;
    ortho.m[10] = -1.0f;
    ortho.m[12] = -1.0f;
    ortho.m[13] = 1.0f;
    ortho.m[15] = 1.0f;
    uiShader->setMat4("uProjection", ortho);

    float cx = screenWidth / 2.0f;
    float cy = screenHeight / 2.0f;

    // 1. Crosshair in screen center
    drawRect(cx - 8, cy - 1, 16, 2, 1.0f, 1.0f, 1.0f, 0.8f);
    drawRect(cx - 1, cy - 8, 2, 16, 1.0f, 1.0f, 1.0f, 0.8f);

    // 2. Hotbar container (9 slots)
    float hotbarW = 360.0f;
    float hotbarH = 40.0f;
    float hotbarX = cx - hotbarW / 2.0f;
    float hotbarY = screenHeight - hotbarH - 10.0f;

    // Hotbar background
    drawRect(hotbarX, hotbarY, hotbarW, hotbarH, 0.1f, 0.1f, 0.1f, 0.6f);

    // 9 Slot outlines
    float slotW = hotbarW / 9.0f;
    for (int i = 0; i < 9; ++i) {
        float sx = hotbarX + i * slotW;
        if (i == selectedSlot) {
            // Selected slot highlight
            drawRect(sx, hotbarY, slotW, hotbarH, 1.0f, 1.0f, 1.0f, 0.4f);
            drawRect(sx, hotbarY, slotW, 2, 1.0f, 1.0f, 1.0f, 0.9f);
            drawRect(sx, hotbarY + hotbarH - 2, slotW, 2, 1.0f, 1.0f, 1.0f, 0.9f);
        } else {
            drawRect(sx, hotbarY, 1, hotbarH, 0.3f, 0.3f, 0.3f, 0.5f);
        }
    }

    // 3. Health bar (10 hearts)
    float healthBarX = hotbarX;
    float healthBarY = hotbarY - 20.0f;
    for (int i = 0; i < 10; ++i) {
        float hx = healthBarX + i * 16.0f;
        if (health >= (i + 1) * 2.0f) {
            drawRect(hx, healthBarY, 12, 12, 0.9f, 0.1f, 0.1f, 0.9f); // Full heart
        } else if (health >= i * 2.0f + 1.0f) {
            drawRect(hx, healthBarY, 6, 12, 0.9f, 0.1f, 0.1f, 0.9f);  // Half heart
        } else {
            drawRect(hx, healthBarY, 12, 12, 0.2f, 0.2f, 0.2f, 0.4f); // Empty heart
        }
    }

    // 4. Food bar (10 food icons)
    float foodBarX = hotbarX + hotbarW - 160.0f;
    for (int i = 0; i < 10; ++i) {
        float fx = foodBarX + (9 - i) * 16.0f;
        if (food >= (i + 1) * 2) {
            drawRect(fx, healthBarY, 12, 12, 0.7f, 0.4f, 0.1f, 0.9f); // Full food
        } else {
            drawRect(fx, healthBarY, 12, 12, 0.2f, 0.2f, 0.2f, 0.4f); // Empty
        }
    }

    uiShader->unbind();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void Hud::addChatMessage(const std::string& message) {
    chatHistory.push_back(message);
    if (chatHistory.size() > 50) {
        chatHistory.erase(chatHistory.begin());
    }
}

} // namespace mc
