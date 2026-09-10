#pragma once

#include "glm/ext/vector_float3.hpp"
#include "utils.h"

class Material {
  public:
    virtual ~Material() = default;

    virtual Color brdf(const glm::vec3 &wo, const glm::vec3 &wi) const = 0;
};

class LambertMaterial : public Material {
  private:
    glm::vec3 albedo_{};

  public:
    LambertMaterial(const glm::vec3 &albedo) : albedo_(albedo) {
    }

    Color brdf(const glm::vec3 &wo, const glm::vec3 &wi) const override;

    const glm::vec3 &get_albedo() const {
        return albedo_;
    }
};
