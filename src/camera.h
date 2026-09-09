#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "primitive.h"

class Camera {
  private:
    glm::vec3 position_{};
    glm::mat4 combined_matrix{};
    glm::mat4 inv_combined_matrix{};

  public:
    void init(const glm::vec3 &p,
              const glm::vec3 &target,
              const glm::vec3 &up,
              float fov,
              float n,
              float f,
              int W,
              int H);

    Ray get_ray(int x, int y) const;
};