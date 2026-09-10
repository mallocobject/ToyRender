#include "scene_object.h"

SceneObject::SceneObject(const glm::vec3 &postion,
                         const glm::vec3 &euler,
                         const glm::vec3 &scale) {

    auto T = glm::translate(glm::mat4{1.0f}, postion);

    auto R = glm::eulerAngleXYZ(euler.x, euler.y, euler.z);

    auto S = glm::scale(glm::mat4{1.0f}, scale);

    obj2world = T * R * S;
    world2obj = glm::inverse(obj2world);
}

bool SceneObject::intersect(Ray &ray, Intersection &isect) const {
    bool hit = false;
    for (auto &&primitive : primitives_) {
        if (primitive->intersect(ray, isect)) {
            ray.maxt = isect.t;
            hit = true;
        }
    }

    return hit;
}