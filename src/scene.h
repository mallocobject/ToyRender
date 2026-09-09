#pragma once

#include "camera.h"
#include "scene_object.hpp"
#include <memory>
#include <vector>

class Scene {
  private:
    Camera camera_;
    std::vector<std::shared_ptr<SceneObject>> scene_objects_;

  public:
    Scene(const glm::vec3 &p,
          const glm::vec3 &target,
          const glm::vec3 &up,
          float fov,
          float n,
          float f,
          int W,
          int H)
        : camera_(p, target, up, fov, n, f, W, H) {
    }

    const Camera &get_camera() const {
        return camera_;
    }

    std::shared_ptr<SceneObject> create_scene_object(const glm::vec3 &postion,
                                                     const glm::vec3 &euler,
                                                     const glm::vec3 &scale);

    std::shared_ptr<SceneObject> intersect(Ray &ray, Intersection &isect) const;
};
