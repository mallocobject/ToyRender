#pragma once

#include "glm/ext/vector_float3.hpp"

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "intersection.h"
#include "ray.h"
#include <cfloat>

inline Ray operator*(const glm::mat4 &m, const Ray &r) {
    Ray result;
    result.o = glm::vec3{m * glm::vec4{r.o, 1.f}};
    result.d = glm::vec3{m * glm::vec4{r.d, 0.f}};
    result.mint = r.mint;
    result.maxt = r.maxt;
    return result;
}

class SceneObject;

class Primitive {
  protected:
    SceneObject *parent_{nullptr};

  public:
    Primitive(SceneObject *parent) : parent_(parent) {
    }

    virtual ~Primitive() = default;

    virtual bool intersect(const Ray &ray, Intersection &isect) const = 0;
};