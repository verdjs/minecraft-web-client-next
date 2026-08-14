#pragma once
#include <string>
#include "../math/Math.hpp"
#include <glad/glad.h>

namespace mc {

enum class MobType {
    Zombie,
    Skeleton,
    Creeper,
    Pig,
    Cow,
    Sheep
};

class Mob {
public:
    int entityId{0};
    MobType type{MobType::Pig};
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    float yaw{0.0f};
    float pitch{0.0f};

    float limbSwing{0.0f};
    float limbSwingAmount{0.0f};

    Mob(int id, MobType type, const Vec3& pos);
    ~Mob();

    void update(float dt, const Vec3& targetPos);
    void render(const Mat4& viewProjMatrix);

private:
    GLuint vao{0}, vbo{0}, ebo{0};
    size_t indexCount{0};
    void initMesh();
};

} // namespace mc
