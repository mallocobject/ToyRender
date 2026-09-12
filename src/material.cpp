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
    // 余弦加权半球采样：pdf(w) = cos(theta) / π
    const glm::vec3 wi_local = cosine_sample_hemisphere();
    const float cos_theta = std::max(wi_local.z, 0.f);

    return MaterialSample{
        .wi_local = wi_local,
        .pdf = std::max(cos_theta * INV_PI, 1e-6f),
    };
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

    return MaterialSample{.wi_local = wi_local, .pdf = 1.f};
}

float DielectricSpecularMaterial::fresnel(float eta_i,
                                          float eta_t,
                                          float cos_i,
                                          float cos_t) const {
    float r1 =
        (eta_t * cos_i - eta_i * cos_t) / (eta_t * cos_i + eta_i * cos_t);
    float r2 =
        (eta_i * cos_i - eta_t * cos_t) / (eta_i * cos_i + eta_t * cos_t);

    return 0.5f * (r1 * r1 + r2 * r2);
}

void DielectricSpecularMaterial::select_eta(float direction_z,
                                            float &eta_i,
                                            float &eta_t) const {
    // 局部 +Z 一侧视为外侧空气；local -Z 一侧视为内侧介质。
    eta_i = eta_t = 1.f;
    (direction_z > 0.f ? eta_t : eta_i) = eta_;
}

Color DielectricSpecularMaterial::brdf(const glm::vec3 &wo,
                                       const glm::vec3 &wi) const {
    // 完美镜面反射方向：wi = (-wo.x, -wo.y, wo.z)
    const glm::vec3 reflected{-wo.x, -wo.y, wo.z};
    if (!nearly_equal(wi, reflected, 1e-6f)) {
        return Color{0.f};
    }

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

    const Color reflectance = Color{Fr} * reflection_tint_;
    return reflectance / cos_i;
}

Color DielectricSpecularMaterial::btdf(const glm::vec3 &wo,
                                       const glm::vec3 &wi) const {
    // wo: 从表面指向相机；wi: 采样得到的光 incident 方向。
    float eta_i = 1.f;
    float eta_t = 1.f;
    select_eta(wo.z, eta_i, eta_t);

    // wi 必须是 wo 对应的折射方向。
    auto expected_wi = compute_refract_vec(wo, eta_i, eta_t);
    if (!expected_wi || !nearly_equal(wi, *expected_wi, 1e-6f)) {
        return Color{0.f};
    }

    const float cos_out = std::max(std::fabs(wo.z), 1e-6f);
    const float cos_in = std::max(std::fabs(wi.z), 1e-6f);

    // 实际光路与相机路径相反：eta_t -> eta_i。
    // Fresnel 反射率具有互易性，这里传参顺序不影响数值。
    const float Fr = fresnel(eta_t, eta_i, cos_in, cos_out);

    // 反向相机路径的 radiance 传输因子：(eta_i / eta_t)^2
    const float eta_ratio = eta_i / eta_t;

    const Color branch =
        Color{1.f - Fr} * eta_ratio * eta_ratio * transmission_color_;

    return branch / cos_in;
}

std::optional<MaterialSample>
DielectricSpecularMaterial::sample_reflection(const glm::vec3 &wo_local) const {
    const glm::vec3 wi_local{-wo_local.x, -wo_local.y, wo_local.z};
    return MaterialSample{.wi_local = wi_local, .pdf = 1.f};
}

std::optional<MaterialSample>
DielectricSpecularMaterial::sample_refraction(const glm::vec3 &wo_local) const {
    float eta_i = 1.f;
    float eta_t = 1.f;
    select_eta(wo_local.z, eta_i, eta_t);

    auto wi_local = compute_refract_vec(wo_local, eta_i, eta_t);
    if (!wi_local) {
        return std::nullopt; // 全反射，没有折射光
    }

    return MaterialSample{.wi_local = *wi_local, .pdf = 1.f};
}
