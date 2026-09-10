#pragma once

#include "camera.h"
#include "light.h"
#include "material.h"
#include "scene_object.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Scene {
  private:
    Camera camera_;
    std::vector<std::shared_ptr<SceneObject>> scene_objects_;
    std::vector<std::shared_ptr<Light>> lights_;
    std::unordered_map<std::string, std::shared_ptr<Material>> materials_;

  public:
    Scene(const glm::vec3 &p,
          const glm::vec3 &target,
          const glm::vec3 &up,
          float fov,
          float n,
          float f,
          int w,
          int h)
        : camera_(p, target, up, fov, n, f, w, h) {
    }

    const Camera &get_camera() const {
        return camera_;
    }

    const std::vector<std::shared_ptr<Light>> &get_lights() const {
        return lights_;
    }

    std::shared_ptr<Material> get_materials(const std::string &key) const {
        if (auto it = materials_.find(key); it != materials_.end()) {
            return it->second;
        }

        return nullptr;
    }

    std::shared_ptr<SceneObject> create_scene_object(const glm::vec3 &postion,
                                                     const glm::vec3 &euler,
                                                     const glm::vec3 &scale);

    template <typename T, typename... Args>
    auto create_light(Args &&...args) {
        std::shared_ptr<Light> p =
            std::make_shared<T>(std::forward<Args>(args)...);
        lights_.push_back(p);
        return p;
    }

    // 同名材质只创建一个实例，后续调用返回同一个 shared_ptr。
    template <typename T, typename... Args>
    std::shared_ptr<Material> create_material(std::string key, Args &&...args) {
        if (auto it = materials_.find(key); it != materials_.end()) {
            return it->second;
        }

        auto material = std::make_shared<T>(std::forward<Args>(args)...);
        materials_.emplace(std::move(key), material);
        return material;
    }

    std::shared_ptr<SceneObject> intersect(Ray &ray, Intersection &isect) const;
};
