#pragma once

#include "glm/ext/vector_float3.hpp"
#include "primitive.h"

class SceneObject;

class Triangle : public Primitive {
  private:
    glm::vec3 vertices_[3]{};
    glm::vec3 normal_{};

  public:
    Triangle(SceneObject &parent,
             const glm::vec3 &v0,
             const glm::vec3 &v1,
             const glm::vec3 &v2);

    bool intersect(const Ray &ray, Intersection &isect) const override;
    PrimitiveSample sample() const override;
};