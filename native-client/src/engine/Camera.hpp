#pragma once
#include "../math/Math.hpp"

namespace mc {

class Camera {
public:
    Vec3 position{0.0f, 64.0f, 0.0f};
    float yaw{0.0f};    // in degrees
    float pitch{0.0f};  // in degrees
    float fov{70.0f};   // in degrees
    float nearPlane{0.05f};
    float farPlane{500.0f};

    Camera() = default;

    Vec3 getForward() const;
    Vec3 getRight() const;
    Vec3 getUp() const;

    Mat4 getViewMatrix() const;
    Mat4 getProjectionMatrix(float aspectRatio) const;

    void updateEulerAngles(float deltaYaw, float deltaPitch);
};

} // namespace mc
