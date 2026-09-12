#include "scene_object.h"
#include "utils.h"
#include <algorithm>
#include <cassert>

namespace {

std::shared_ptr<Material> default_scene_material() {
    static std::shared_ptr<Material> material =
        std::make_shared<LambertMaterial>(Color{0.8f, 0.8f, 0.8f});
    return material;
}

} // namespace

SceneObject::SceneObject(const glm::vec3 &postion,
                         const glm::vec3 &euler,
                         const glm::vec3 &scale)
    : material_(default_scene_material()) {

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

PrimitiveSample SceneObject::sample() const {
    if (primitives_.empty()) {
        return PrimitiveSample{};
    }

    int lucky = random_int(0, primitives_.size() - 1);
    auto ps = primitives_[lucky]->sample();

    float sum_area = 0.f;
    std::ranges::for_each(primitives_,
                          [&sum_area](auto &&p) { sum_area += p->area(); });
    assert(sum_area > 0.f);

    float pdf = primitives_[lucky]->area() / sum_area;

    ps.pdf *= pdf;

    return ps;
}