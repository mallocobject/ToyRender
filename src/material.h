#pragma once

#include "glm/ext/vector_float3.hpp"
#include "utils.h"

#include <cassert>
#include <optional>

struct MaterialSample {
    glm::vec3 wi_local{};
    float pdf{1.f};
};

class Material {
  protected:
    Color emissive_{0.f};

  public:
    Material(const Color &emissive = Color{0.f}) : emissive_(emissive) {
    }

    virtual ~Material() = default;

    const Color &get_emissive() const {
        return emissive_;
    }

    virtual Color brdf(const glm::vec3 &, const glm::vec3 &) const = 0;

    virtual Color btdf(const glm::vec3 &, const glm::vec3 &) const {
        return Color{0.f};
    }

    // 在局部切线空间采样一个反射/散射方向，并返回 L * f * cos / pdf
    // 中的权重。
    virtual std::optional<MaterialSample>
    sample_reflection(const glm::vec3 &) const = 0;

    virtual std::optional<MaterialSample>
    sample_refraction(const glm::vec3 &) const {
        return std::nullopt;
    }

    virtual constexpr bool is_delta() const {
        return false;
    }

    virtual constexpr bool is_refractable() const {
        return false;
    }
};

class LambertMaterial : public Material {
  private:
    glm::vec3 albedo_{};

  public:
    LambertMaterial(const glm::vec3 &albedo, const Color &emissive = Color{0.f})
        : Material(emissive), albedo_(albedo) {
    }

    Color brdf(const glm::vec3 &wo, const glm::vec3 &wi) const override;

    std::optional<MaterialSample>
    sample_reflection(const glm::vec3 &wo_local) const override;

    const glm::vec3 &get_albedo() const {
        return albedo_;
    }
};

class ConductorSpecularMaterial : public Material {
  private:
    Color eta_{};
    Color absorption_{0.f};
    Color reflection_tint_{1.f};

  public:
    ConductorSpecularMaterial(const Color &eta,
                              const Color &absorption,
                              const Color &reflection_tint,
                              const Color &emissive = Color{0.f})
        : Material(emissive), eta_(eta), absorption_(absorption),
          reflection_tint_(reflection_tint) {
    }

    Color brdf(const glm::vec3 &wo, const glm::vec3 &wi) const override;

    std::optional<MaterialSample>
    sample_reflection(const glm::vec3 &wo_local) const override;

    const Color &get_eta() const {
        return eta_;
    }

    const Color &get_absorption() const {
        return absorption_;
    }

    const Color &get_reflection_tint() const {
        return reflection_tint_;
    }

    bool constexpr is_delta() const override {
        return true;
    }

  private:
    Color fresnel(float cos_theta) const;
};

class DielectricSpecularMaterial : public Material {
  private:
    float eta_{};
    Color transmission_color_{1.f};
    Color reflection_tint_{1.f};

  public:
    DielectricSpecularMaterial(float eta,
                               const Color &transmission_color,
                               const Color &reflection_tint,
                               const Color &emissive = Color{0.f})
        : Material(emissive), eta_(eta),
          transmission_color_(transmission_color),
          reflection_tint_(reflection_tint) {
        assert(eta > 0);
    }

    Color brdf(const glm::vec3 &wo, const glm::vec3 &wi) const override;
    Color btdf(const glm::vec3 &wo, const glm::vec3 &wi) const override;

    std::optional<MaterialSample>
    sample_reflection(const glm::vec3 &wo_local) const override;

    std::optional<MaterialSample>
    sample_refraction(const glm::vec3 &wo_local) const override;

    bool constexpr is_delta() const override {
        return true;
    }

    bool constexpr is_refractable() const override {
        return true;
    }

  private:
    float fresnel(float eta_i, float eta_t, float cos_i, float cos_t) const;

    // 局部 +Z 一侧为外侧介质，-Z 一侧为内侧介质。
    void select_eta(float direction_z, float &eta_i, float &eta_t) const;
};
