#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "primitive.h"
#include <algorithm>
#include <cfloat>
#include <memory>
#include <utility>
#include <vector>

#include <glm/ext/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

using Color = glm::vec3;

class SceneObject {
  private:
    glm::mat4 obj2world{};
    glm::mat4 world2obj{};

    std::vector<std::unique_ptr<Primitive>> primitives_;

  public:
    SceneObject(const glm::vec3 &postion,
                const glm::vec3 &euler,
                const glm::vec3 &scale);

    ~SceneObject(){};

    const auto &get_obj2world() const {
        return obj2world;
    }

    const auto &get_world2obj() const {
        return world2obj;
    }

    template <typename T, typename... Args>
    void create_object(Args &&...args) {
        auto p = std::make_unique<T>(this, std::forward<Args>(args)...);
        primitives_.push_back(std::move(p));
    }

    bool intersect(const Ray &ray, Intersection &isect) const;
};

inline SceneObject::SceneObject(const glm::vec3 &postion,
                                const glm::vec3 &euler,
                                const glm::vec3 &scale) {

    auto T = glm::translate(glm::mat4{1.0f}, postion);

    auto R = glm::eulerAngleXYZ(euler.x, euler.y, euler.z);

    auto S = glm::scale(glm::mat4{1.0f}, scale);

    obj2world = T * R * S;
}

inline bool SceneObject::intersect(const Ray &ray, Intersection &isect) const {
    bool hit = false;
    for (Intersection tmp_isect{}; auto &&primitive : primitives_) {
        if (primitive->intersect(ray, tmp_isect) && tmp_isect.t < isect.t) {
            isect = tmp_isect;
            hit = true;
        }
    }

    return hit;
}