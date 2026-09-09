#include "render.h"
#include "disk.h"
#include "glm/common.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"
#include "primitive.h"
#include "scene.h"
#include "scene_object.hpp"
#include "sphere.h"
#include "tinyxml2.h"
#include "triangle.h"
#include <MiniFB.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <random>
#include <ranges>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace rg = std::ranges;
namespace vws = std::views;

using namespace std::literals;

namespace {

glm::vec3 parse_vec3(const char *text, const glm::vec3 &fallback) {
    if (text == nullptr) {
        return fallback;
    }

    std::string str(text);
    std::replace(str.begin(), str.end(), ',', ' ');
    std::istringstream iss(str);

    float values[3] = {0.f, 0.f, 0.f};
    int count = 0;
    while (count < 3 && (iss >> values[count])) {
        ++count;
    }

    if (count == 0) {
        return fallback;
    }

    if (count == 1) {
        values[1] = values[2] = values[0];
    } else if (count == 2) {
        values[2] = fallback.z;
    }

    return glm::vec3{values[0], values[1], values[2]};
}

float parse_float(const char *text, float fallback) {
    if (text == nullptr) {
        return fallback;
    }

    char *end = nullptr;
    float value = std::strtof(text, &end);
    return end == text ? fallback : value;
}

const char *first_attribute(tinyxml2::XMLElement *element,
                            std::initializer_list<const char *> names) {
    for (const char *name : names) {
        const char *value = element->Attribute(name);
        if (value != nullptr) {
            return value;
        }
    }
    return nullptr;
}

void add_primitive_to_scene_object(tinyxml2::XMLElement *primitive,
                                   SceneObject *scene_object) {
    const char *name = primitive->Name();
    if (name == nullptr) {
        return;
    }

    const std::string type(name);

    if (type == "triangle") {
        glm::vec3 vertex_0 = parse_vec3(
            first_attribute(primitive, {"v0", "p0"}), glm::vec3{0.f});
        glm::vec3 vertex_1 = parse_vec3(
            first_attribute(primitive, {"v1", "p1"}), glm::vec3{0.f});
        glm::vec3 vertex_2 = parse_vec3(
            first_attribute(primitive, {"v2", "p2"}), glm::vec3{0.f});
        scene_object->create_object<Triangle>(vertex_0, vertex_1, vertex_2);
    } else if (type == "sphere") {
        float radius = parse_float(primitive->Attribute("radius"), 1.f);
        scene_object->create_object<Sphere>(radius);
    } else if (type == "disk") {
        float radius = parse_float(primitive->Attribute("radius"), 1.f);
        scene_object->create_object<Disk>(radius);
    }
}

} // namespace

std::unique_ptr<Scene> Render::create_default_scene(int w, int h) const {
    auto scene = std::make_unique<Scene>(glm::vec3{0.f, 0.f, 0.f},
                                         glm::vec3{0.f, 0.f, 1.f},
                                         glm::vec3{0.f, 1.f, 0.f},
                                         glm::radians(60.f),
                                         0.1f,
                                         1000.f,
                                         w,
                                         h);

    auto scene_object_1 = scene->create_scene_object(
        glm::vec3{0.f, 0.f, 5.f}, glm::vec3{0.f}, glm::vec3{2.f});

    scene_object_1->create_object<Triangle>(glm::vec3{-1.f, -1.f, 0.f},
                                            glm::vec3{1.f, -1.f, 0.f},
                                            glm::vec3{1.f, 1.f, 0.f});
    scene_object_1->create_object<Triangle>(glm::vec3{-1.f, -1.f, 0.f},
                                            glm::vec3{1.f, 1.f, 0.f},
                                            glm::vec3{-1.f, 1.f, 0.f});

    auto scene_object_2 = scene->create_scene_object(
        glm::vec3{0.f, 0.f, 2.f}, glm::vec3{0.f}, glm::vec3{1.f});
    scene_object_2->create_object<Sphere>(0.5f);

    return scene;
}

Render::Render(int w,
               int h,
               int sample_per_pixel,
               const std::string &scene_file)
    : viewport_width_(w), viewport_height_(h),
      sample_per_pixel_(sample_per_pixel) {
    buffer_ = new uint32_t[w * h * 4](0);

    scene_ = create_default_scene(w, h);
    load_scene_from_xml(scene_file, w, h);
}

bool Render::load_scene_from_xml(const std::string &scene_file, int w, int h) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(scene_file.c_str()) != tinyxml2::XML_SUCCESS) {
        return false;
    }

    tinyxml2::XMLElement *root = doc.RootElement();
    if (root == nullptr) {
        return false;
    }

    glm::vec3 camera_position{0.f, 0.f, 0.f};
    glm::vec3 camera_target{0.f, 0.f, 1.f};
    glm::vec3 camera_up{0.f, 1.f, 0.f};
    float fov_degrees = 60.f;
    float near_plane = 0.1f;
    float far_plane = 1000.f;

    if (tinyxml2::XMLElement *camera = root->FirstChildElement("camera")) {
        camera_position = parse_vec3(
            first_attribute(camera, {"position", "eye", "lookFrom", "origin"}),
            glm::vec3{0.f, 0.f, 0.f});
        camera_target =
            parse_vec3(first_attribute(camera, {"target", "lookAt", "to"}),
                       glm::vec3{0.f, 0.f, 1.f});
        camera_up =
            parse_vec3(camera->Attribute("up"), glm::vec3{0.f, 1.f, 0.f});
        fov_degrees = parse_float(camera->Attribute("fov"), 60.f);
        near_plane =
            parse_float(first_attribute(camera, {"near", "nearPlane"}), 0.1f);
        far_plane =
            parse_float(first_attribute(camera, {"far", "farPlane"}), 1000.f);
    }

    auto new_scene = std::make_unique<Scene>(camera_position,
                                             camera_target,
                                             camera_up,
                                             glm::radians(fov_degrees),
                                             near_plane,
                                             far_plane,
                                             w,
                                             h);

    for (tinyxml2::XMLElement *object = root->FirstChildElement("object");
         object != nullptr;
         object = object->NextSiblingElement("object")) {
        glm::vec3 position = parse_vec3(
            first_attribute(object, {"position", "center", "origin"}),
            glm::vec3{0.f});
        glm::vec3 rotation = parse_vec3(
            first_attribute(object, {"rotation", "euler"}), glm::vec3{0.f});
        glm::vec3 scale =
            parse_vec3(object->Attribute("scale"), glm::vec3{1.f});

        auto scene_object =
            new_scene->create_scene_object(position, rotation, scale);

        for (tinyxml2::XMLElement *primitive = object->FirstChildElement();
             primitive != nullptr;
             primitive = primitive->NextSiblingElement()) {
            add_primitive_to_scene_object(primitive, scene_object.get());
        }
    }

    for (tinyxml2::XMLElement *element = root->FirstChildElement();
         element != nullptr;
         element = element->NextSiblingElement()) {
        const char *name = element->Name();
        if (name == nullptr || std::string(name) == "camera" ||
            std::string(name) == "object") {
            continue;
        }

        const std::string type(name);
        if (type != "triangle" && type != "sphere" && type != "disk") {
            continue;
        }

        glm::vec3 position = parse_vec3(
            first_attribute(element, {"position", "center", "origin"}),
            glm::vec3{0.f});
        glm::vec3 rotation = parse_vec3(
            first_attribute(element, {"rotation", "euler"}), glm::vec3{0.f});
        glm::vec3 scale =
            parse_vec3(element->Attribute("scale"), glm::vec3{1.f});

        auto scene_object =
            new_scene->create_scene_object(position, rotation, scale);
        add_primitive_to_scene_object(element, scene_object.get());
    }

    scene_ = std::move(new_scene);
    return true;
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
    Ray ray = scene_->get_camera().get_ray(px, py);
    Intersection isect{};
    if (scene_->intersect(ray, isect)) {
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
