#include "material.h"
#include "glm/ext/vector_float3.hpp"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace {

bool nearly_equal(const glm::vec3 &a, const glm::vec3 &b, float epsilon) {
    return std::fabs(a.x - b.x) < epsilon && std::fabs(a.y - b.y) < epsilon &&
           std::fabs(a.z - b.z) < epsilon;
}

} // namespace

Color LambertMaterial::brdf(const glm::vec3 &, const glm::vec3 &) const {
    return albedo_ * INV_PI;
}

std::optional<MaterialSample>
LambertMaterial::sample_reflection(const glm::vec3 &) const {
    // 余弦加权采样：
    // pdf(wi) = cosθ / π
    // weight  = f * cosθ / pdf
    //         = (albedo / π) * cosθ / (cosθ / π)
    //         = albedo
    return MaterialSample{.wi_local = sample_cosine_hemisphere(),
                          .weight = albedo_};
}

Color ConductorSpecularMaterial::fresnel(float cos_theta) const {
    cos_theta = std::max(std::fabs(cos_theta), 1e-6f);

    const Color absorption = absorption_;
    const Color eta_2_plus_k_2 = eta_ * eta_ + absorption * absorption;

    const Color r1 =
        (eta_2_plus_k_2 * cos_theta * cos_theta - 2.f * eta_ * cos_theta +
         1.f) /
        (eta_2_plus_k_2 * cos_theta * cos_theta + 2.f * eta_ * cos_theta + 1.f);

    const Color r2 =
        (eta_2_plus_k_2 - 2.f * eta_ * cos_theta + cos_theta * cos_theta) /
        (eta_2_plus_k_2 + 2.f * eta_ * cos_theta + cos_theta * cos_theta);

    return (r1 + r2) * 0.5f;
}

Color ConductorSpecularMaterial::brdf(const glm::vec3 &wo,
                                      const glm::vec3 &wi) const {
    // 完美镜面方向：wi = (-wo.x, -wo.y, wo.z)
    if (!nearly_equal(wi, glm::vec3{-wo.x, -wo.y, wo.z}, 1e-6f)) {
        return Color{0.f};
    }

    const float cos_theta = std::max(std::fabs(wi.z), 1e-6f);
    return fresnel(cos_theta) * reflection_tint_ / cos_theta;
}

std::optional<MaterialSample>
ConductorSpecularMaterial::sample_reflection(const glm::vec3 &wo_local) const {
    // 完美镜面反射，反射概率为 1。
    const glm::vec3 wi_local{-wo_local.x, -wo_local.y, wo_local.z};

    // delta BRDF 的估计权重 = f * cosθ / pdf
    // pdf = 1，f = Fr * reflection_color / cosθ
    // => weight = Fr * reflection_color
    const float cos_theta = std::max(std::fabs(wi_local.z), 1e-6f);
    const Color weight = fresnel(cos_theta) * reflection_tint_;

    return MaterialSample{.wi_local = wi_local, .weight = weight};
}

float DielectricSpeculerMaterial::fresnel(float eta_i,
                                          float eta_t,
                                          float cos_i,
                                          float cos_t) const {
    float r1 =
        (eta_t * cos_i - eta_i * cos_t) / (eta_t * cos_i + eta_i * cos_t);
    float r2 =
        (eta_i * cos_i - eta_t * cos_t) / (eta_i * cos_i + eta_t * cos_t);

    return 0.5f * (r1 * r1 + r2 * r2);
}

void DielectricSpeculerMaterial::select_eta(float direction_z,
                                            float &eta_i,
                                            float &eta_t) const {
    // 局部 +Z 一侧视为外侧空气；local -Z 一侧视为内侧介质。
    eta_i = eta_t = 1.f;
    (direction_z > 0.f ? eta_t : eta_i) = eta_;
}

Color DielectricSpeculerMaterial::brdf(const glm::vec3 &wo,
                                       const glm::vec3 &wi) const {
    // 完美镜面反射方向：wi = (-wo.x, -wo.y, wo.z)
    // const glm::vec3 reflected{-wo.x, -wo.y, wo.z};
    // if (!nearly_equal(wi, reflected, 1e-6f)) {
    //     return Color{0.f};
    // }

    // 反射不跨界面，入射侧和相机侧相同。
    float eta_i = 1.f;
    float eta_t = 1.f;
    select_eta(wo.z, eta_i, eta_t);

    const float cos_i = std::max(std::fabs(wi.z), 1e-6f);

    float Fr = 1.f;
    if (auto wt = compute_refract_vec(wi, eta_i, eta_t)) {
        const float cos_t = std::max(std::fabs(wt->z), 1e-6f);
        Fr = fresnel(eta_i, eta_t, cos_i, cos_t);
    }

    return Color{Fr} * reflection_tint_ / cos_i;
}

Color DielectricSpeculerMaterial::btdf(const glm::vec3 &wt,
                                       const glm::vec3 &wi) const {
    // 实际光方向 wi：从表面指向光源。
    // +Z 一侧为空气，-Z 一侧为介质。
    float eta_i = 1.f;
    float eta_t = 1.f;
    select_eta(wi.z, eta_i, eta_t);

    // auto expected_wt = compute_refract_vec(wi, eta_i, eta_t);
    // if (!expected_wt) {
    //     return Color{0.f};
    // }

    // if (!nearly_equal(wt, *expected_wt, 1e-6f)) {
    //     return Color{0.f};
    // }

    const float cos_i = std::max(std::fabs(wi.z), 1e-6f);
    const float cos_t = std::max(std::fabs(wt.z), 1e-6f);

    const float Fr = fresnel(eta_i, eta_t, cos_i, cos_t);
    const float eta_ratio = eta_t / eta_i;

    return Color{1.f - Fr} * eta_ratio * eta_ratio * transmission_color_ /
           cos_i;
}

std::optional<MaterialSample>
DielectricSpeculerMaterial::sample_reflection(const glm::vec3 &wo_local) const {
    const glm::vec3 wi_local{-wo_local.x, -wo_local.y, wo_local.z};

    float eta_i = 1.f;
    float eta_t = 1.f;
    select_eta(wo_local.z, eta_i, eta_t);

    const float cos_i = std::max(std::fabs(wo_local.z), 1e-6f);

    // 默认按全反射处理；只有能折射时才用 Fresnel 修正反射率。
    float Fr = 1.f;
    if (auto wt_local = compute_refract_vec(wo_local, eta_i, eta_t)) {
        const float cos_t = std::max(std::fabs(wt_local->z), 1e-6f);
        Fr = fresnel(eta_i, eta_t, cos_i, cos_t);
    }

    return MaterialSample{.wi_local = wi_local,
                          .weight = Color{Fr} * reflection_tint_};
}

std::optional<MaterialSample>
DielectricSpeculerMaterial::sample_refraction(const glm::vec3 &wo_local) const {
    float eta_i = 1.f;
    float eta_t = 1.f;
    select_eta(wo_local.z, eta_i, eta_t);

    auto wi_local = compute_refract_vec(wo_local, eta_i, eta_t);
    if (!wi_local) {
        return std::nullopt; // 全反射，没有折射光
    }

    const float cos_i = std::max(std::fabs(wo_local.z), 1e-6f);
    const float cos_t = std::max(std::fabs(wi_local->z), 1e-6f);
    const float Fr = fresnel(eta_i, eta_t, cos_i, cos_t);

    // 这里 eta_i / eta_t 是“相机侧介质 / 折射后介质”。
    // 因为 compute_refract_vec 是沿相机路径反向计算，
    // radiance 传输因子应为 (eta_i / eta_t)^2。
    const float eta_ratio = eta_i / eta_t;

    return MaterialSample{.wi_local = *wi_local,
                          .weight = Color{1.f - Fr} * eta_ratio * eta_ratio *
                                    transmission_color_};
}
