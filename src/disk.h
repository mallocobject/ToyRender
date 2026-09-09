#pragma once

#include "primitive.h"

class SceneObject;

class Disk : public Primitive {
  private:
    float radius_{0.f};

  public:
    Disk(SceneObject *parent, float radius);
    bool intersect(const Ray &ray, Intersection &isect) const override;
};