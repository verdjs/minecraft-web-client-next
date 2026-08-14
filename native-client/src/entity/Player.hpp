#pragma once
#include "../math/Math.hpp"
#include "../engine/Camera.hpp"

namespace mc {

class World;

class Player {
public:
    Vec3 position{0.0f, 70.0f, 0.0f};
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    Camera camera;

    bool onGround{false};
    bool isSprinting{false};
    bool isSneaking{false};
    bool isFlying{false};

    // Item usage & eating bobbing animation
    bool isUsingItem{false};
    int itemUsageTicks{0};

    // Dimensions
    float width{0.6f};
    float height{1.8f};
    float eyeHeight{1.62f};

    Player();

    void update(float dt, const World& world, bool forward, bool backward, bool left, bool right, bool jump, bool sneak, bool sprint);

    AABB getBoundingBox() const {
        return AABB(
            Vec3(position.x - width / 2.0f, position.y, position.z - width / 2.0f),
            Vec3(position.x + width / 2.0f, position.y + height, position.z + width / 2.0f)
        );
    }

    void startUsingItem() { isUsingItem = true; itemUsageTicks = 0; }
    void stopUsingItem() { isUsingItem = false; itemUsageTicks = 0; }

    // First person eating bobbing offset
    Vec3 getEatingOffset() const;
    Vec3 getEatingRotation() const;
};

} // namespace mc
