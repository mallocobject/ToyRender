#pragma once

#include "camera.h"
#include "scene_object.hpp"
#include <atomic>
#include <cstdint>
#include <glm/vec3.hpp>

using Color = glm::vec3;

class Primitive;

class Render {
  private:
    int viewport_width_{800};
    int viewport_height_{600};
    int sample_per_pixel_{100};

    uint32_t *buffer_{nullptr};
    std::atomic<int> current_row_pixel_index_{0};

    Camera camera_{};

    SceneObject *so_{};

  public:
    Render(int w, int h, int sample_per_pixel = 100);

    ~Render();

    void run();

  private:
    Color render_pixel(int x, int y) const;
    Color render_sub_pixed(float px, float py) const;

    void run_render_thread();
};