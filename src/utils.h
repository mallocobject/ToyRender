#pragma once

#include "glm/ext/scalar_constants.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>

using Color = glm::vec3;

inline constexpr const float PI = glm::pi<float>();
inline constexpr const float INV_PI = 1.f / PI;
inline constexpr const float PI_HALF = PI / 2;
inline constexpr const float PI_DOUBLE = PI * 2;
inline constexpr const float PI_SQUARE = PI * PI;

inline float random_float(float a = 0.f, float b = 1.f) {
    thread_local std::mt19937 rng{std::random_device{}()};
    thread_local std::uniform_real_distribution<float> dist{0.f, 1.f};

    assert(b - a > 1e-6f);

    return dist(rng) * (b - a) + a;
}

// radian
inline constexpr glm::vec3 spherical_coordinate(float theta, float phi) {
    return glm::vec3{std::sin(theta) * std::cos(phi),
                     std::sin(theta) * std::sin(phi),
                     std::cos(theta)};
}

// 按余弦权重在 normal 所朝半球内采样，返回世界空间单位方向。
inline glm::vec3 sample_cosine_hemisphere(const glm::vec3 &normal) {
    const glm::vec3 n = glm::normalize(normal);

    const float r1 = random_float();
    const float r2 = random_float();

    const float phi = PI_DOUBLE * r2;
    const float sin_theta = std::sqrt(r1);
    const float cos_theta = std::sqrt(std::max(0.f, 1.f - r1));

    const glm::vec3 local_dir{
        sin_theta * std::cos(phi),
        sin_theta * std::sin(phi),
        cos_theta,
    };

    const glm::vec3 up = std::fabs(n.z) < 0.999f ? glm::vec3{0.f, 0.f, 1.f}
                                                 : glm::vec3{1.f, 0.f, 0.f};
    const glm::vec3 tangent = glm::normalize(glm::cross(up, n));
    const glm::vec3 bitangent = glm::cross(n, tangent);

    return glm::normalize(tangent * local_dir.x + bitangent * local_dir.y +
                          n * local_dir.z);
}