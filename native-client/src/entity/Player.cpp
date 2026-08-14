#include "Player.hpp"
#include "../world/World.hpp"
#include <cmath>
#include <algorithm>

namespace mc {

Player::Player() {
    camera.position = position + Vec3(0, eyeHeight, 0);
}

void Player::update(float dt, const World& world, bool forward, bool backward, bool left, bool right, bool jump, bool sneak, bool sprint) {
    if (isUsingItem) {
        itemUsageTicks++;
    }

    isSprinting = sprint && forward && !sneak;
    isSneaking = sneak;

    float speed = isSprinting ? 8.6f : (isSneaking ? 2.5f : 4.3f);
    if (isFlying) speed *= 3.0f;

    Vec3 forwardDir = camera.getForward();
    forwardDir.y = 0.0f;
    forwardDir = forwardDir.normalized();

    Vec3 rightDir = camera.getRight();
    rightDir.y = 0.0f;
    rightDir = rightDir.normalized();

    Vec3 moveDir(0, 0, 0);
    if (forward) moveDir += forwardDir;
    if (backward) moveDir -= forwardDir;
    if (right) moveDir += rightDir;
    if (left) moveDir -= rightDir;

    if (moveDir.lengthSq() > 0.001f) {
        moveDir = moveDir.normalized();
        velocity.x = moveDir.x * speed;
        velocity.z = moveDir.z * speed;
    } else {
        velocity.x *= 0.6f;
        velocity.z *= 0.6f;
    }

    if (!isFlying) {
        // Apply Gravity
        velocity.y -= 28.0f * dt;
        if (velocity.y < -50.0f) velocity.y = -50.0f; // Terminal velocity

        if (jump && onGround) {
            velocity.y = 9.0f;
            onGround = false;
        }
    } else {
        velocity.y = 0.0f;
        if (jump) velocity.y = speed;
        if (sneak) velocity.y = -speed;
    }

    // Collision detection & resolution with voxel blocks
    Vec3 delta = velocity * dt;

    // Y Axis collision
    position.y += delta.y;
    AABB boxY = getBoundingBox();
    int minX = std::floor(boxY.min.x);
    int maxX = std::floor(boxY.max.x);
    int minY = std::floor(boxY.min.y);
    int maxY = std::floor(boxY.max.y);
    int minZ = std::floor(boxY.min.z);
    int maxZ = std::floor(boxY.max.z);

    onGround = false;
    for (int y = minY; y <= maxY; ++y) {
        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                if (world.getBlock(x, y, z) != BlockType::Air && getBlockInfo(world.getBlock(x, y, z)).isSolid) {
                    if (delta.y < 0.0f) {
                        position.y = y + 1.0f;
                        velocity.y = 0.0f;
                        onGround = true;
                    } else if (delta.y > 0.0f) {
                        position.y = y - height;
                        velocity.y = 0.0f;
                    }
                }
            }
        }
    }

    // X Axis collision
    position.x += delta.x;
    AABB boxX = getBoundingBox();
    minX = std::floor(boxX.min.x);
    maxX = std::floor(boxX.max.x);
    minY = std::floor(boxX.min.y);
    maxY = std::floor(boxX.max.y);
    minZ = std::floor(boxX.min.z);
    maxZ = std::floor(boxX.max.z);

    for (int y = minY; y <= maxY; ++y) {
        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                if (world.getBlock(x, y, z) != BlockType::Air && getBlockInfo(world.getBlock(x, y, z)).isSolid) {
                    if (delta.x > 0.0f) {
                        position.x = x - width / 2.0f;
                        velocity.x = 0.0f;
                    } else if (delta.x < 0.0f) {
                        position.x = x + 1.0f + width / 2.0f;
                        velocity.x = 0.0f;
                    }
                }
            }
        }
    }

    // Z Axis collision
    position.z += delta.z;
    AABB boxZ = getBoundingBox();
    minX = std::floor(boxZ.min.x);
    maxX = std::floor(boxZ.max.x);
    minY = std::floor(boxZ.min.y);
    maxY = std::floor(boxZ.max.y);
    minZ = std::floor(boxZ.min.z);
    maxZ = std::floor(boxZ.max.z);

    for (int y = minY; y <= maxY; ++y) {
        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                if (world.getBlock(x, y, z) != BlockType::Air && getBlockInfo(world.getBlock(x, y, z)).isSolid) {
                    if (delta.z > 0.0f) {
                        position.z = z - width / 2.0f;
                        velocity.z = 0.0f;
                    } else if (delta.z < 0.0f) {
                        position.z = z + 1.0f + width / 2.0f;
                        velocity.z = 0.0f;
                    }
                }
            }
        }
    }

    camera.position = position + Vec3(0, isSneaking ? eyeHeight - 0.2f : eyeHeight, 0);
}

Vec3 Player::getEatingOffset() const {
    if (!isUsingItem || itemUsageTicks <= 0) return {0, 0, 0};

    float progress = std::min(static_cast<float>(itemUsageTicks) / 32.0f, 1.0f);
    float mouthFactor = 1.0f - std::pow(1.0f - std::min(progress * 2.0f, 1.0f), 3.0f);

    float chewCycle = static_cast<float>(itemUsageTicks % 4) / 4.0f;
    float chew = progress < 0.85f ? std::abs(std::sin(chewCycle * 3.14159265f)) * 0.06f : 0.0f;

    return {
        0.08f * mouthFactor,
        (0.12f + chew) * mouthFactor,
        0.18f * mouthFactor
    };
}

Vec3 Player::getEatingRotation() const {
    if (!isUsingItem || itemUsageTicks <= 0) return {0, 0, 0};

    float progress = std::min(static_cast<float>(itemUsageTicks) / 32.0f, 1.0f);
    float mouthFactor = 1.0f - std::pow(1.0f - std::min(progress * 2.0f, 1.0f), 3.0f);

    float chewCycle = static_cast<float>(itemUsageTicks % 4) / 4.0f;
    float chew = progress < 0.85f ? std::abs(std::sin(chewCycle * 3.14159265f)) * 0.06f : 0.0f;

    return {
        35.0f * mouthFactor,
        -20.0f * mouthFactor,
        15.0f * mouthFactor + (chew * 80.0f)
    };
}

} // namespace mc
