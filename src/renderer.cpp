#include "renderer.h"
#include "glm/common.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "intersection.h"
#include "material.h"
#include "ray.h"
#include "scene_loader.h"
#include "utils.h"
#include <MiniFB.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <ranges>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace rg = std::ranges;
namespace vws = std::views;

using namespace std::literals;

Renderer::Renderer(int w,
                   int h,
                   int sample_per_pixel,
                   int min_depth,
                   int max_depth,
                   const std::string &scene_file)
    : viewport_width_(w), viewport_height_(h),
      sample_per_pixel_(sample_per_pixel), min_depth_(min_depth),
      max_depth_(max_depth) {
    buffer_ = std::make_unique<uint32_t[]>(static_cast<size_t>(w) *
                                           static_cast<size_t>(h) * 4u);

    if (auto loaded_scene =
            SceneLoader::load_scene_from_xml(scene_file, w, h)) {
        scene_ = std::move(loaded_scene);
    } else {
        scene_ = SceneLoader::create_default_scene(w, h);
    }
}

void Renderer::run() {
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

        state = mfb_update_ex(
            window, buffer_.get(), viewport_width_, viewport_height_);

        if (state != MFB_STATE_OK) {
            break;
        }

    } while (mfb_wait_sync(window));

    // rg::for_each(threads, [](auto &&thread) { thread.join(); });
    window = nullptr;
}

Color Renderer::render_pixel(int x, int y) const {

    Color color{0.f};
    rg::for_each(vws::iota(0, sample_per_pixel_), [&color, x, y, this](auto) {
        float px = x + random_float();
        float py = y + random_float();
        color +=
            render_sub_pixed(px, py) / static_cast<float>(sample_per_pixel_);
    });

    return color;
}

Color Renderer::render_sub_pixed(float px, float py) const {
    Ray ray = scene_->get_camera().get_ray(px, py);
    Color color = get_radiance(ray);

    return color;
}

void Renderer::run_render_thread() {
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

Color Renderer::get_radiance(Ray &ray, int depth) const {
    if (depth >= max_depth_) {
        return Color{0.f};
    }

    static constexpr const float survival_probability = 0.8f;
    float reward_factor = 1.f;

    if (depth >= min_depth_) {
        if (float gambling = random_float(); gambling > survival_probability) {
            return Color{0.f};
        }
        reward_factor = 1.f / survival_probability;
    }

    Intersection isect{};

    std::shared_ptr<SceneObject> hit_object = scene_->intersect(ray, isect);
    if (!hit_object) {
        return Color{0.f};
    }

    const std::shared_ptr<Material> &material = hit_object->get_material();
    const glm::vec3 wo = -glm::normalize(ray.d);

    // 基本元素保证 normal 是真正的表面法线方向，因此不再翻向观察方向。
    const glm::vec3 normal = glm::normalize(isect.normal);

    const TangentFrame tangent_frame = build_tangent_frame(normal);
    const glm::vec3 wo_local = to_local(wo, tangent_frame);
    Color Io{0.f};

    constexpr const float epsilon = 1e-4f;

    if (material && !material->is_specularable()) { // directional lighting
        for (auto &&light : scene_->get_lights()) {
            glm::vec3 light_sample_position{0.f};
            const Color L =
                light->get_radiance(isect.postion, light_sample_position);
            if (L.r == 0.f && L.g == 0.f && L.b == 0.f) {
                continue;
            }

            const glm::vec3 to_light = light_sample_position - isect.postion;
            const float light_distance = glm::length(to_light);
            if (light_distance <= epsilon) {
                continue;
            }

            const glm::vec3 wi = to_light / light_distance;

            const float cos_theta = glm::max(glm::dot(normal, wi), 0.f);
            if (cos_theta <= 0.f) {
                continue;
            }

            Ray shadow_ray{.o = isect.postion,
                           .d = wi,
                           .mint = epsilon,
                           .maxt = light_distance - epsilon};

            if (shadow_ray.maxt <= shadow_ray.mint) {
                continue;
            }

            if (Intersection shadow_isect{};
                scene_->intersect(shadow_ray, shadow_isect)) {
                continue;
            }

            const glm::vec3 wi_local = to_local(wi, tangent_frame);
            const Color f =
                material ? material->brdf(wo_local, wi_local) : Color{1.f};
            Io += L * f * cos_theta;
        }
    }

    if (material) { // indirect lighting
        // 由材质自己决定采样方向：
        // - Lambert：余弦加权半球采样
        // - ConductorSpecular：完美镜面反射
        // const std::optional<MaterialSample> sample =
        // material->sample(wo_local);

        if (auto sample = material->sample_reflection(wo_local)) {
            // 局部采样方向 -> 世界空间，用于投射反弹射线
            const glm::vec3 wi =
                glm::normalize(to_world(sample->wi_local, tangent_frame));

            Ray bounce_ray{
                .o = isect.postion,
                .d = wi,
                .mint = epsilon,
            };
            const Color L = get_radiance(bounce_ray, depth + 1);

            // sample->weight 已经是 f * cosθ / pdf
            Io += L * sample->weight;
        }

        if (auto sample = material->sample_refraction(wo_local)) {
            // 局部采样方向 -> 世界空间，用于投射反弹射线
            const glm::vec3 wi =
                glm::normalize(to_world(sample->wi_local, tangent_frame));

            Ray bounce_ray{
                .o = isect.postion,
                .d = wi,
                .mint = epsilon,
            };
            const Color L = get_radiance(bounce_ray, depth + 1);

            // sample->weight 已经是 f * cosθ / pdf
            Io += L * sample->weight;
        }
    }

    return Io * reward_factor;
}
