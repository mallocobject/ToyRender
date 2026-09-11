#pragma once

#include "ray.h"
#include "scene.h"
#include "utils.h"
#include <atomic>
#include <cstdint>
#include <glm/vec3.hpp>
#include <memory>
#include <string>

class Renderer {
  private:
    int viewport_width_{800};
    int viewport_height_{600};
    int sample_per_pixel_{100};
    int max_depth_{10};
    int min_depth_{3};

    std::unique_ptr<uint32_t[]> buffer_{};
    std::atomic<int> current_row_pixel_index_{0};

    std::unique_ptr<Scene> scene_;

  public:
    Renderer(int w,
             int h,
             int sample_per_pixel = 100,
             int min_depth = 3,
             int max_depth = 10,
             const std::string &scene_file = "scenes/scene02.xml");

    ~Renderer() = default;

    void run();

  private:
    Color render_pixel(int x, int y) const;
    Color render_sub_pixed(float px, float py) const;
    Color get_radiance(Ray &ray, int depth = 0) const;

    void run_render_thread();
};
