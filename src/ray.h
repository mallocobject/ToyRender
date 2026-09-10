#pragma once

#include "glm/ext/vector_float3.hpp"
#include <cfloat>

struct Ray {
    glm::vec3 o{};
    glm::vec3 d{};

    float mint = 0.f;
    float maxt = FLT_MAX;
};