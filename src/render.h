#pragma once

#include "scene.h"
#include "scene_object.hpp"
#include <atomic>
#include <cstdint>
#include <glm/vec3.hpp>
#include <memory>
#include <string>

using Color = glm::vec3;

class Primitive;

class Render {
  private:
    int viewport_width_{800};
    int viewport_height_{600};
    int sample_per_pixel_{100};

    uint32_t *buffer_{nullptr};
    std::atomic<int> current_row_pixel_index_{0};

    std::unique_ptr<Scene> scene_;

  public:
    Render(int w,
           int h,
           int sample_per_pixel = 100,
           const std::string &scene_file = "scenes/scenes.xml");

    ~Render();

    void run();

  private:
    Color render_pixel(int x, int y) const;
    Color render_sub_pixed(float px, float py) const;

    void run_render_thread();

    bool load_scene_from_xml(const std::string &scene_file, int w, int h);
    std::unique_ptr<Scene> create_default_scene(int w, int h) const;
};
