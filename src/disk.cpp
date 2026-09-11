#include "disk.h"
#include "glm/ext/vector_float4.hpp"
#include "primitive.h"
#include "scene_object.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

Disk::Disk(SceneObject &parent, float radius)
    : Primitive(parent), radius_(radius) {
}

bool Disk::intersect(const Ray &ray, Intersection &isect) const {
    Ray r = parent_.get_world2obj() * ray;

    if (std::fabs(r.o.z) < 1e-6f) {
        return false;
    }

    float t = -r.o.z / r.d.z;

    if (t < r.mint || t > r.maxt) {
        return false;
    }

    auto p = r.o + t * r.d;

    if (glm::dot(p, p) > radius_ * radius_) {
        return false;
    }

    isect.t = t;
    isect.postion = parent_.get_obj2world() * glm::vec4{p, 1.f};
    isect.normal =
        glm::normalize(parent_.get_obj2world() * glm::vec4{0.f, 0.f, 1.f, 0.f});

    return true;
}