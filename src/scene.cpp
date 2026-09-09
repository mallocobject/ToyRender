#include "scene.h"

std::shared_ptr<SceneObject> Scene::create_scene_object(
    const glm::vec3 &postion, const glm::vec3 &euler, const glm::vec3 &scale) {
    auto scene_object = std::make_shared<SceneObject>(postion, euler, scale);
    scene_objects_.push_back(scene_object);

    return scene_object;
}

std::shared_ptr<SceneObject> Scene::intersect(Ray &ray,
                                              Intersection &isect) const {
    std::shared_ptr<SceneObject> hit_scene_object;
    for (auto &&scene_object : scene_objects_) {
        if (scene_object->intersect(ray, isect)) {
            ray.maxt = isect.t;
            hit_scene_object = scene_object;
        }
    }

    return hit_scene_object;
}