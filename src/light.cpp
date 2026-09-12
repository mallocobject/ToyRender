#include "light.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "scene_object.h"
#include <cmath>

LightSample DirectionalLight::get_radiance(const glm::vec3 &p) const {

    return LightSample{
        .s = p - direction_ * 1e6f,
        .radiance = radiance_,
    };
}

LightSample PointLight::get_radiance(const glm::vec3 &p) const {
    float r = glm::length(p - position_);
    float div = glm::dot(attenuations_, glm::vec3{r * r, r, 1});
    if (std::fabs(div) < 1e-6f) {
        return LightSample{
            .s = position_,
            .radiance = Color{0.f},
        };
    }

    return LightSample{
        .s = position_,
        .radiance = intensity_ / div,
    };
}

LightSample SpotLight::get_radiance(const glm::vec3 &p) const {
    LightSample ls = PointLight::get_radiance(p);

    glm::vec3 op = glm::normalize(p - position_);
    float cos_theta = glm::dot(op, direction_);

    float k2 =
        (cos_outer_angle_ - cos_theta) / (cos_outer_angle_ - cos_inner_angle_);

    ls.radiance *= glm::clamp(k2, 0.f, 1.f);

    return ls;
}

LightSample AreaLight::get_radiance(const glm::vec3 &p) const {
    const std::shared_ptr<SceneObject> scene_object = scene_object_.lock();
    if (!scene_object) {
        return LightSample{.radiance = Color{0.f}};
    }

    PrimitiveSample ps = scene_object->sample();

    const glm::vec3 to_light = ps.p - p;
    const float r2 = glm::dot(to_light, to_light);
    if (r2 < 1e-6f) {
        return LightSample{
            .s = ps.p,
            .radiance = Color{0.f},
        };
    }

    const float r = std::sqrt(r2);
    const glm::vec3 wi = to_light / r;

    const float cos_light = glm::dot(-wi, ps.normal);
    if (cos_light < 1e-6f) {
        return LightSample{
            .s = ps.p,
            .radiance = Color{0.f},
        };
    }

    const std::shared_ptr<Material> &material = scene_object->get_material();
    const Color emissive = material ? material->get_emissive() : Color{0.f};

    return LightSample{
        .s = ps.p,
        .radiance = emissive,
        .pdf = r2 * ps.pdf / cos_light,
    };
}

std::shared_ptr<SceneObject> AreaLight::get_scene_object() const {
    return scene_object_.lock();
}
