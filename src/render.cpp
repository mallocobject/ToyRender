#include "render.h"
#include "disk.h"
#include "glm/common.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"
#include "primitive.h"
#include "scene_object.hpp"
#include "sphere.h"
#include "triangle.h"
#include <MiniFB.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <ranges>
#include <thread>
#include <vector>

namespace rg = std::ranges;
namespace vws = std::views;

using namespace std::literals;

Render::Render(int w, int h, int sample_per_pixel)
    : viewport_width_(w), viewport_height_(h),
      sample_per_pixel_(sample_per_pixel) {
    buffer_ = new uint32_t[w * h * 4](0);
    camera_.init(glm::vec3{0.f, 0.f, 0.f},
                 glm::vec3{0.f, 0.f, 1.f},
                 glm::vec3{0.f, 1.f, 0.f},
                 glm::radians(60.f),
                 0.1f,
                 1000.f,
                 w,
                 h);

    so_ = new SceneObject(
        glm::vec3{0.f, 0.f, 5.f}, glm::vec3{0.f}, glm::vec3{1.f});

    so_->create_object<Triangle>(glm::vec3{-1.f, 0.f, 0.f},
                                 glm::vec3{0.f, 1.f, 0.f},
                                 glm::vec3{1.f, 0.f, 0.f});
    so_->create_object<Triangle>(glm::vec3{-1.f, 0.f, 0.f},
                                 glm::vec3{0.f, -1.f, 0.f},
                                 glm::vec3{1.f, 0.f, 0.f});
}

Render::~Render() {
    if (buffer_) {
        delete[] buffer_;
        buffer_ = nullptr;
    }
}

void Render::run() {
    mfb_window *window = mfb_open_ex(
        "ToyRender", viewport_width_, viewport_height_, MFB_WF_RESIZABLE);
    if (window == nullptr) {
        return;
    }

    int num_threads = std::thread::hardware_concurrency();
    std::vector<std::jthread> threads(num_threads);

    rg::for_each(threads, [this](auto &&thread) {
        thread = std::jthread([this] { run_render_thread(); });
    });

    mfb_update_state state;
    do {
        // TODO: add some fancy rendering to the buffer of size viewport_width_
        // * viewport_height_

        state =
            mfb_update_ex(window, buffer_, viewport_width_, viewport_height_);

        if (state != MFB_STATE_OK) {
            break;
        }

    } while (mfb_wait_sync(window));

    // rg::for_each(threads, [](auto &&thread) { thread.join(); });
    window = nullptr;
}

Color Render::render_pixel(int x, int y) const {
    // std::this_thread::sleep_for(1ms);

    thread_local std::mt19937 rng{std::random_device{}()};
    thread_local std::uniform_real_distribution<float> dist{0.f, 1.f};

    Color color{0.f};
    rg::for_each(vws::iota(0, sample_per_pixel_), [&color, x, y, this](auto) {
        float px = x + dist(rng);
        float py = y + dist(rng);
        color +=
            render_sub_pixed(px, py) / static_cast<float>(sample_per_pixel_);
    });

    return color;
}

Color Render::render_sub_pixed(float px, float py) const {

    Color color{0.f};
    Ray ray = camera_.get_ray(px, py);
    Intersection isect{};
    if (so_->intersect(ray, isect)) {
        color = isect.normal * 0.5f + 0.5f;
    }

    return color;
}

void Render::run_render_thread() {
    while (true) {
        int row_pixel_index = current_row_pixel_index_.fetch_add(1);
        if (row_pixel_index >= viewport_height_) {
            return;
        }

        rg::for_each(
            vws::iota(0, viewport_width_), [row_pixel_index, this](int x) {
                int y = row_pixel_index;
                Color color = render_pixel(x, y);
                uint32_t r = glm::clamp(
                    static_cast<uint32_t>(std::round(color.r * 255.f)),
                    0u,
                    255u);
                uint32_t g = glm::clamp(
                    static_cast<uint32_t>(std::round(color.g * 255.f)),
                    0u,
                    255u);
                uint32_t b = glm::clamp(
                    static_cast<uint32_t>(std::round(color.b * 255.f)),
                    0u,
                    255u);

                buffer_[y * viewport_width_ + x] = (r << 16) | (g << 8) | b;
            });
    }
}