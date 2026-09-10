#include "material.h"
#include "utils.h"

Color LambertMaterial::brdf(const glm::vec3 &wo, const glm::vec3 &wi) const {
    (void)wo;
    (void)wi;
    return albedo_ * INV_PI;
}