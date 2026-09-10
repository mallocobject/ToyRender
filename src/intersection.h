#pragma once

#include "glm/ext/vector_float3.hpp"
#include <cfloat>
struct Intersection {
    glm::vec3 postion{};
    glm::vec3 normal{};
    float t{FLT_MAX};
};
