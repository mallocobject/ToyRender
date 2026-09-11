#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "intersection.h"
#include "material.h"
#include "primitive.h"
#include "ray.h"
#include <algorithm>
#include <cfloat>
#include <memory>
#include <utility>
#include <vector>

#include <glm/ext/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/euler_angles.hpp>

class SceneObject {
  private:
    glm::mat4 obj2world{};
    glm::mat4 world2obj{};

    std::vector<std::unique_ptr<Primitive>> primitives_;
    std::shared_ptr<Material> material_{};

  public:
    SceneObject(const glm::vec3 &postion,
                const glm::vec3 &euler,
                const glm::vec3 &scale);

    ~SceneObject() = default;

    const auto &get_obj2world() const {
        return obj2world;
    }

    const auto &get_world2obj() const {
        return world2obj;
    }

    template <typename T, typename... Args>
    void create_object(Args &&...args) {
        auto p = std::make_unique<T>(*this, std::forward<Args>(args)...);
        primitives_.push_back(std::move(p));
    }

    bool intersect(Ray &ray, Intersection &isect) const;

    void set_material(std::shared_ptr<Material> material) {
        material_ = std::move(material);
    }

    const std::shared_ptr<Material> &get_material() const {
        return material_;
    }
};
