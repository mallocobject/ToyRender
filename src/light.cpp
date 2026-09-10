#include "light.h"

Color DirectionalLight::get_radiance(const glm::vec3 &p, glm::vec3 &s) const {
    s = p - direction_ * 1e6f;
    return radiance_;
}

Color PointLight::get_radiance(const glm::vec3 &p, glm::vec3 &s) const {
    s = position_;
    float r = glm::length(p - position_);
    float div = glm::dot(attenuations_, glm::vec3{r * r, r, 1});
    if (std::fabs(div) < 1e-6f) {
        return Color{0.f};
    }

    return intensity_ / div;
}

Color SpotLight::get_radiance(const glm::vec3 &p, glm::vec3 &s) const {
    Color radiance = PointLight::get_radiance(p, s);

    glm::vec3 op = glm::normalize(p - position_);
    float theta_cos = glm::dot(op, direction_);

    float k2 =
        (outer_angle_cos_ - theta_cos) / (outer_angle_cos_ - inner_angle_cos_);

    return radiance * glm::clamp(k2, 0.f, 1.f);
}