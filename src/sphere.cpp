#include "sphere.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "primitive.h"
#include "scene_object.h"
#include <cmath>

Sphere::Sphere(SceneObject &parent, float radius)
    : Primitive(parent), radius_(radius) {
}

bool Sphere::intersect(const Ray &ray, Intersection &isect) const {
    Ray r = parent_.get_world2obj() * ray;

    float A = glm::dot(r.d, r.d);
    float B = 2.f * glm::dot(r.o, r.d);
    float C = glm::dot(r.o, r.o) - radius_ * radius_;

    float delta = B * B - 4.f * A * C;
    if (delta < 0.f) {
        return false;
    }

    float sqrt_delta = sqrtf(delta);
    float t1 = (-B - sqrt_delta) / (2.f * A);
    float t2 = (-B + sqrt_delta) / (2.f * A);

    float t = t1;

    if (t1 < r.mint && t2 >= r.mint && t2 <= r.maxt) {
        t = t2;
    } else if (t1 >= r.mint && t1 <= r.maxt) {
        t = t1;
    } else {
        return false;
    }

    glm::vec3 p = r.o + t * r.d;
    glm::vec3 n = glm::normalize(p);
    isect.t = t;
    isect.postion = parent_.get_obj2world() * glm::vec4{p, 1.f};
    isect.normal = glm::normalize(parent_.get_obj2world() * glm::vec4{n, 0.f});

    return true;
}