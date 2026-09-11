#include "triangle.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "primitive.h"
#include "scene_object.h"
#include <cmath>

Triangle::Triangle(SceneObject &parent,
                   const glm::vec3 &v0,
                   const glm::vec3 &v1,
                   const glm::vec3 &v2)
    : Primitive(parent) {
    vertices_[0] = parent_.get_obj2world() * glm::vec4{v0, 1.f};
    vertices_[1] = parent_.get_obj2world() * glm::vec4{v1, 1.f};
    vertices_[2] = parent_.get_obj2world() * glm::vec4{v2, 1.f};

    auto edge1 = vertices_[1] - vertices_[0];
    auto edge2 = vertices_[2] - vertices_[0];

    normal_ = glm::normalize(glm::cross(edge1, edge2));
}

bool Triangle::intersect(const Ray &ray, Intersection &isect) const {
    auto p0 = vertices_[0];
    auto p1 = vertices_[1];
    auto p2 = vertices_[2];

    auto e1 = p1 - p0;
    auto e2 = p2 - p0;
    auto s = ray.o - p0;
    auto s1 = glm::cross(ray.d, e2);
    auto s2 = glm::cross(s, e1);

    float det = glm::dot(s1, e1);
    if (std::fabs(det) < 1e-6f) {
        return false;
    }

    float inv_det = 1.f / det;

    float b1 = glm::dot(s1, s) / det;
    float b2 = glm::dot(s2, ray.d) / det;
    float t = glm::dot(s2, e2) / det;

    if (t < ray.mint || t > ray.maxt) {
        return false;
    }

    float b0 = 1.f - b1 - b2;
    if (b0 < 0.f || b1 < 0.f || b2 < 0.f) {
        return false;
    }

    isect.t = t;
    isect.postion = ray.o + t * ray.d;
    isect.normal = normal_;

    return true;
}