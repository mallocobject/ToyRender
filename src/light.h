#pragma once

#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "utils.h"
#include <cassert>
#include <cmath>
class Light {
  public:
    virtual Color get_radiance(const glm::vec3 &p, glm::vec3 &s) const = 0;
};

class DirectionalLight : public Light {
  private:
    glm::vec3 direction_{};
    Color radiance_{};

  public:
    DirectionalLight(const glm::vec3 &direction, const Color &radiance)
        : direction_(glm::normalize(direction)), radiance_(radiance) {
    }

    Color get_radiance(const glm::vec3 &p, glm::vec3 &s) const override;
};

class PointLight : public Light {
  protected:
    glm::vec3 position_{};
    Color intensity_{};
    glm::vec3 attenuations_{}; // a, b, c

  public:
    PointLight(const glm::vec3 &position,
               const Color &intensity,
               const glm::vec3 &attenuations)
        : position_(position), intensity_(intensity),
          attenuations_(attenuations) {
    }

    Color get_radiance(const glm::vec3 &p, glm::vec3 &s) const override;
};

class SpotLight : public PointLight {
  private:
    glm::vec3 direction_{};
    float inner_angle_cos_{};
    float outer_angle_cos_{};

  public:
    SpotLight(const glm::vec3 &position,
              const glm::vec3 &direction,
              const Color &intensity,
              float inner_angle,
              float outer_angle,
              const glm::vec3 &attenuations)
        : PointLight(position, intensity, attenuations),
          direction_(glm::normalize(direction)),
          inner_angle_cos_(std::cos(inner_angle)),
          outer_angle_cos_(std::cos(outer_angle)) {
        assert(inner_angle_cos_ - outer_angle_cos_ > 1e-6f);
    }

    Color get_radiance(const glm::vec3 &p, glm::vec3 &s) const override;
};