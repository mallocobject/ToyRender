#include "camera.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"

void Camera::init(const glm::vec3 &p,
                  const glm::vec3 &target,
                  const glm::vec3 &up,
                  float fov,
                  float n,
                  float f,
                  int W,
                  int H) {
    position_ = p;
    auto view_matrix = glm::lookAtLH(p, target, up);
    auto proj_matrix = glm::perspectiveFovLH_ZO(
        fov, static_cast<float>(W), static_cast<float>(H), n, f);
    auto vp_matrix = glm::mat4{W / 2.f,
                               0.f,
                               0.f,
                               0.f,
                               0.f,
                               -H / 2.f,
                               0.f,
                               0.f,
                               0.f,
                               0.f,
                               1.f,
                               0.f,
                               W / 2.f,
                               H / 2.f,
                               0.f,
                               1.f};

    combined_matrix = vp_matrix * proj_matrix * view_matrix;
    inv_combined_matrix = glm::inverse(combined_matrix);
}

Ray Camera::get_ray(int x, int y) const {
    Ray ray{};
    ray.o = position_;

    glm::vec4 pos{x, y, 0.f, 1.f};
    auto world_pos = inv_combined_matrix * pos;
    world_pos /= world_pos.w;
    ray.d = glm::normalize(static_cast<glm::vec3>(world_pos) - position_);

    return ray;
}