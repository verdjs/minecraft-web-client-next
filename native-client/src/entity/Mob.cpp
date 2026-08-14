#include "Mob.hpp"
#include <cmath>
#include <vector>

namespace mc {

Mob::Mob(int id, MobType type, const Vec3& pos)
    : entityId(id), type(type), position(pos) {
    initMesh();
}

Mob::~Mob() {
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
}

void Mob::update(float dt, const Vec3& targetPos) {
    // Simple pathing towards target
    Vec3 diff = targetPos - position;
    diff.y = 0.0f;
    float dist = diff.length();

    if (dist > 2.0f && dist < 24.0f) {
        Vec3 dir = diff.normalized();
        float speed = 2.2f;
        velocity.x = dir.x * speed;
        velocity.z = dir.z * speed;
        yaw = std::atan2(dir.x, dir.z) * 180.0f / 3.14159265f;
    } else {
        velocity.x *= 0.5f;
        velocity.z *= 0.5f;
    }

    position += velocity * dt;

    // Update limb swing cycle
    float horizontalSpeed = std::hypot(velocity.x, velocity.z);
    limbSwingAmount += (horizontalSpeed - limbSwingAmount) * std::min(1.0f, dt * 10.0f);
    if (limbSwingAmount > 0.01f) {
        limbSwing += horizontalSpeed * dt * 4.0f;
    }
}

void Mob::initMesh() {
    // Create a 3D box model (body, head, and 4 legs)
    std::vector<float> vertices;
    std::vector<uint32_t> indices;

    auto addBox = [&](float minX, float minY, float minZ, float maxX, float maxY, float maxZ, float u0, float v0, float u1, float v1) {
        uint32_t base = static_cast<uint32_t>(vertices.size() / 8);

        // 8 Corners
        float V[8][3] = {
            {minX, minY, minZ}, {maxX, minY, minZ}, {maxX, maxY, minZ}, {minX, maxY, minZ},
            {minX, minY, maxZ}, {maxX, minY, maxZ}, {maxX, maxY, maxZ}, {minX, maxY, maxZ}
        };

        // 6 Quads
        int faces[6][4] = {
            {5, 4, 0, 1}, // Bottom
            {2, 3, 7, 6}, // Top
            {0, 4, 7, 3}, // West
            {1, 0, 3, 2}, // North
            {5, 1, 2, 6}, // East
            {4, 5, 6, 7}  // South
        };

        for (int f = 0; f < 6; ++f) {
            for (int i = 0; i < 4; ++i) {
                int vi = faces[f][i];
                vertices.push_back(V[vi][0]);
                vertices.push_back(V[vi][1]);
                vertices.push_back(V[vi][2]);
                vertices.push_back(u0);
                vertices.push_back(v0);
                vertices.push_back(0);
                vertices.push_back(1);
                vertices.push_back(0);
            }
            indices.push_back(base + f * 4 + 0);
            indices.push_back(base + f * 4 + 1);
            indices.push_back(base + f * 4 + 2);
            indices.push_back(base + f * 4 + 2);
            indices.push_back(base + f * 4 + 3);
            indices.push_back(base + f * 4 + 0);
        }
    };

    // Body
    addBox(-0.3f, 0.4f, -0.4f, 0.3f, 0.9f, 0.4f, 0.0f, 0.0f, 1.0f, 1.0f);
    // Head
    addBox(-0.25f, 0.6f, -0.7f, 0.25f, 1.1f, -0.3f, 0.0f, 0.0f, 1.0f, 1.0f);
    // 4 Legs
    addBox(-0.3f, 0.0f, -0.35f, -0.1f, 0.4f, -0.15f, 0.0f, 0.0f, 1.0f, 1.0f);
    addBox(0.1f, 0.0f, -0.35f, 0.3f, 0.4f, -0.15f, 0.0f, 0.0f, 1.0f, 1.0f);
    addBox(-0.3f, 0.0f, 0.15f, -0.1f, 0.4f, 0.35f, 0.0f, 0.0f, 1.0f, 1.0f);
    addBox(0.1f, 0.0f, 0.15f, 0.3f, 0.4f, 0.35f, 0.0f, 0.0f, 1.0f, 1.0f);

    indexCount = indices.size();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void Mob::render(const Mat4& viewProjMatrix) {
    if (vao == 0 || indexCount == 0) return;
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace mc
