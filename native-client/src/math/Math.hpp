#pragma once
#include <cmath>
#include <array>
#include <algorithm>

namespace mc {

struct Vec2 {
    float x{0.0f}, y{0.0f};
    constexpr Vec2() = default;
    constexpr Vec2(float x, float y) : x(x), y(y) {}
};

struct Vec3 {
    float x{0.0f}, y{0.0f}, z{0.0f};
    constexpr Vec3() = default;
    constexpr Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }

    float lengthSq() const { return x * x + y * y + z * z; }
    float length() const { return std::sqrt(lengthSq()); }
    Vec3 normalized() const {
        float l = length();
        return l > 0.00001f ? *this / l : Vec3(0, 0, 0);
    }
    static float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
};

struct Vec3i {
    int x{0}, y{0}, z{0};
    constexpr Vec3i() = default;
    constexpr Vec3i(int x, int y, int z) : x(x), y(y), z(z) {}
    bool operator==(const Vec3i& o) const { return x == o.x && y == o.y && z == o.z; }
};

struct Mat4 {
    std::array<float, 16> m{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    static Mat4 identity() { return Mat4(); }

    static Mat4 perspective(float fovRad, float aspect, float nearZ, float farZ) {
        Mat4 res{};
        float tanHalfFov = std::tan(fovRad / 2.0f);
        res.m[0] = 1.0f / (aspect * tanHalfFov);
        res.m[5] = 1.0f / tanHalfFov;
        res.m[10] = -(farZ + nearZ) / (farZ - nearZ);
        res.m[11] = -1.0f;
        res.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
        res.m[15] = 0.0f;
        return res;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalized();
        Vec3 s = Vec3::cross(f, up).normalized();
        Vec3 u = Vec3::cross(s, f);

        Mat4 res{};
        res.m[0] = s.x; res.m[4] = s.y; res.m[8] = s.z;
        res.m[1] = u.x; res.m[5] = u.y; res.m[9] = u.z;
        res.m[2] = -f.x; res.m[6] = -f.y; res.m[10] = -f.z;
        res.m[12] = -Vec3::dot(s, eye);
        res.m[13] = -Vec3::dot(u, eye);
        res.m[14] = Vec3::dot(f, eye);
        res.m[15] = 1.0f;
        return res;
    }

    Mat4 operator*(const Mat4& r) const {
        Mat4 out{};
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                out.m[i * 4 + j] =
                    m[i * 4 + 0] * r.m[0 * 4 + j] +
                    m[i * 4 + 1] * r.m[1 * 4 + j] +
                    m[i * 4 + 2] * r.m[2 * 4 + j] +
                    m[i * 4 + 3] * r.m[3 * 4 + j];
            }
        }
        return out;
    }

    const float* data() const { return m.data(); }
};

struct AABB {
    Vec3 min;
    Vec3 max;

    constexpr AABB() : min(0, 0, 0), max(0, 0, 0) {}
    constexpr AABB(const Vec3& min, const Vec3& max) : min(min), max(max) {}

    bool intersects(const AABB& o) const {
        return (min.x <= o.max.x && max.x >= o.min.x) &&
               (min.y <= o.max.y && max.y >= o.min.y) &&
               (min.z <= o.max.z && max.z >= o.min.z);
    }
};

} // namespace mc
