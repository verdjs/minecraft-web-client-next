#include "Camera.hpp"
#include <cmath>

namespace mc {

static constexpr float DEG2RAD = 3.14159265358979323846f / 180.0f;

Vec3 Camera::getForward() const {
    float yawRad = yaw * DEG2RAD;
    float pitchRad = pitch * DEG2RAD;

    Vec3 f;
    f.x = std::cos(pitchRad) * std::sin(yawRad);
    f.y = std::sin(pitchRad);
    f.z = -std::cos(pitchRad) * std::cos(yawRad);
    return f.normalized();
}

Vec3 Camera::getRight() const {
    return Vec3::cross(getForward(), Vec3(0, 1, 0)).normalized();
}

Vec3 Camera::getUp() const {
    return Vec3::cross(getRight(), getForward()).normalized();
}

Mat4 Camera::getViewMatrix() const {
    return Mat4::lookAt(position, position + getForward(), getUp());
}

Mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return Mat4::perspective(fov * DEG2RAD, aspectRatio, nearPlane, farPlane);
}

void Camera::updateEulerAngles(float deltaYaw, float deltaPitch) {
    yaw += deltaYaw;
    pitch += deltaPitch;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    while (yaw >= 360.0f) yaw -= 360.0f;
    while (yaw < 0.0f) yaw += 360.0f;
}

} // namespace mc
