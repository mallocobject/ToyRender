#pragma once

#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "utils.h"
#include <cassert>
#include <cmath>
#include <memory>

struct LightSample {
    glm::vec3 s{};
    Color radiance{};
    float pdf{1.f};
};

class Light {
  public:
    virtual ~Light() = default;
    virtual LightSample get_radiance(const glm::vec3 &) const = 0;
};

class DirectionalLight : public Light {
  private:
    glm::vec3 direction_{};
    Color radiance_{};

  public:
    DirectionalLight(const glm::vec3 &direction, const Color &radiance)
        : direction_(glm::normalize(direction)), radiance_(radiance) {
    }

    LightSample get_radiance(const glm::vec3 &p) const override;
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

    LightSample get_radiance(const glm::vec3 &p) const override;
};

class SpotLight : public PointLight {
  private:
    glm::vec3 direction_{};
    float cos_inner_angle_{};
    float cos_outer_angle_{};

  public:
    SpotLight(const glm::vec3 &position,
              const glm::vec3 &direction,
              const Color &intensity,
              float inner_angle,
              float outer_angle,
              const glm::vec3 &attenuations)
        : PointLight(position, intensity, attenuations),
          direction_(glm::normalize(direction)),
          cos_inner_angle_(std::cos(inner_angle)),
          cos_outer_angle_(std::cos(outer_angle)) {
        assert(cos_inner_angle_ - cos_outer_angle_ > 1e-6f);
    }

    LightSample get_radiance(const glm::vec3 &p) const override;
};

class SceneObject;

class AreaLight : public Light {
  private:
    std::weak_ptr<SceneObject> scene_object_;

  public:
    explicit AreaLight(const std::shared_ptr<SceneObject> &scene_object)
        : scene_object_(scene_object) {
        assert(scene_object);
    }

    LightSample get_radiance(const glm::vec3 &p) const override;

    std::shared_ptr<SceneObject> get_scene_object() const;
};