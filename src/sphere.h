#pragma once

#include "primitive.h"

class SceneObject;

class Sphere : public Primitive {
  private:
    float radius_{0.f};

  public:
    Sphere(SceneObject *parent, float radius);
    bool intersect(const Ray &ray, Intersection &isect) const override;
};