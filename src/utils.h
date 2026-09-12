#pragma once

#include "glm/ext/scalar_constants.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <optional>
#include <random>
#include <type_traits>

using Color = glm::vec3;

inline constexpr const float PI = glm::pi<float>();
inline constexpr const float INV_PI = 1.f / PI;
inline constexpr const float HALF_PI = PI / 2.f;
inline constexpr const float DOUBLE_PI = PI * 2.f;
inline constexpr const float SQUARE_PI = PI * PI;

// template <typename T>
// auto random_float(T a = 0.f, T b = 1.f) -> T {
//     thread_local std::mt19937 rng{std::random_device{}()};
//     thread_local std::uniform_real_distribution<float> dist{0.f, 1.f};

//     assert(b - a > 1e-6f);

//     return dist(rng) * (b - a) + a;
// }

namespace details {

template <typename T>
concept Arithmetic = std::is_arithmetic_v<T> && !std::same_as<T, bool>;

template <Arithmetic T>
struct Random {
    T operator()(T a = 0, T b = 1) const {
        thread_local std::mt19937 rng{std::random_device{}()};

        if constexpr (std::integral<T>) {
            assert(a <= b);
            std::uniform_int_distribution<T> dist(a, b);
            return dist(rng);
        } else {
            assert(a < b);
            thread_local std::uniform_real_distribution<T> dist{T{0}, T{1}};
            return a + (b - a) * dist(rng);
        }
    }
};

} // namespace details

inline constexpr details::Random<float> random_float{};
inline constexpr details::Random<int> random_int{};

// radian
inline constexpr glm::vec3 spherical_coordinate(float theta, float phi) {
    return glm::vec3{std::sin(theta) * std::cos(phi),
                     std::sin(theta) * std::sin(phi),
                     std::cos(theta)};
}

struct TangentFrame {
    glm::vec3 tangent{};
    glm::vec3 bitangent{};
    glm::vec3 normal{};
};

// 构造以 normal 为 Z 轴的正交基。
inline TangentFrame build_tangent_frame(const glm::vec3 &normal) {
    TangentFrame frame;
    frame.normal = glm::normalize(normal);

    const glm::vec3 up = std::fabs(frame.normal.z) < 0.999f
                             ? glm::vec3{0.f, 0.f, 1.f}
                             : glm::vec3{1.f, 0.f, 0.f};

    frame.tangent = glm::normalize(glm::cross(up, frame.normal));
    frame.bitangent = glm::cross(frame.normal, frame.tangent);
    return frame;
}

// 世界方向 -> 切线空间局部方向。
inline glm::vec3 to_local(const glm::vec3 &direction,
                          const TangentFrame &frame) {
    return glm::vec3{glm::dot(direction, frame.tangent),
                     glm::dot(direction, frame.bitangent),
                     glm::dot(direction, frame.normal)};
}

// 切线空间局部方向 -> 世界方向。
inline glm::vec3 to_world(const glm::vec3 &direction,
                          const TangentFrame &frame) {
    return frame.tangent * direction.x + frame.bitangent * direction.y +
           frame.normal * direction.z;
}

// local space: normal = +Z
inline std::optional<glm::vec3>
compute_refract_vec(const glm::vec3 &wi_in, float eta_i, float eta_t) {
    if (eta_i <= 0.f || eta_t <= 0.f) {
        return std::nullopt;
    }

    const glm::vec3 wi = glm::normalize(wi_in);

    // 正入射时 cos_i = 1，仍然应该发生折射，不能返回 nullopt。
    const float cos_i = std::clamp(std::fabs(wi.z), 0.f, 1.f);
    const float eta = eta_i / eta_t;

    const float sin2_i = std::max(1.f - cos_i * cos_i, 0.f);
    const float sin2_t = sin2_i * eta * eta;

    // 全反射
    if (sin2_t >= 1.f) {
        return std::nullopt;
    }

    const float cos_t = std::sqrt(1.f - sin2_t);

    glm::vec3 wt = wi * glm::vec3{-eta, -eta, 1.f};
    wt.z = wi.z > 0.f ? -cos_t : cos_t;

    return glm::normalize(wt);
}

inline glm::vec2 uniform_sample_disk(float R) {
    float u1 = random_float();
    float u2 = random_float();

    float r = R * std::sqrt(u1);
    float theta = DOUBLE_PI * u2;

    return glm::vec2{r * std::cos(theta), r * std::sin(theta)};
}

inline glm::vec3 uniform_sample_hemisphere() {
    float u1 = random_float();
    float u2 = random_float();

    float cos_theta = u1; // 1 - u1
    float sin_theta = std::sqrt(1 - cos_theta * cos_theta);
    float phi = DOUBLE_PI * u2;

    float x = sin_theta * std::cos(phi);
    float y = sin_theta * std::sin(phi);
    float z = cos_theta;

    return glm::vec3{x, y, z};
}

inline glm::vec3 uniform_sample_sphere() {
    float u1 = random_float();
    float u2 = random_float();

    float cos_theta = 1 - 2 * u1;
    float sin_theta = std::sqrt(1 - cos_theta * cos_theta);
    float phi = DOUBLE_PI * u2;

    float x = sin_theta * std::cos(phi);
    float y = sin_theta * std::sin(phi);
    float z = cos_theta;

    return glm::vec3{x, y, z};
}

inline glm::vec3 uniform_sample_triangle(const glm::vec3 &p0,
                                         const glm::vec3 &p1,
                                         const glm::vec3 &p2) {
    float u1 = random_float();
    float u2 = random_float();

    float sqrt_u1 = std::sqrt(u1);
    float b1 = 1.f - sqrt_u1;
    float b2 = u2 * sqrt_u1;

    glm::vec3 p = (1.f - b1 - b2) * p0 + b1 * p1 + b2 * p2;

    return p;
}

// 按余弦权重在切线空间 +Z 半球内采样，返回局部方向。
inline glm::vec3 cosine_sample_hemisphere() {
    const float r1 = random_float();
    const float r2 = random_float();

    const float phi = DOUBLE_PI * r2;
    const float sin_theta = std::sqrt(r1);
    const float cos_theta = std::sqrt(std::max(0.f, 1.f - r1));

    return glm::vec3{
        sin_theta * std::cos(phi),
        sin_theta * std::sin(phi),
        cos_theta,
    };
}