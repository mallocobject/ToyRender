#include "scene_loader.h"

#include "disk.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"
#include "material.h"
#include "sphere.h"
#include "tinyxml2.h"
#include "triangle.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <string>

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

std::string normalize_token(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    text.erase(std::remove_if(text.begin(),
                              text.end(),
                              [](unsigned char c) {
                                  return c == '_' || c == '-' ||
                                         std::isspace(c) != 0;
                              }),
               text.end());
    return text;
}

std::string light_type(tinyxml2::XMLElement *element) {
    std::string type;
    if (const char *name = element->Name()) {
        type = name;
    }
    if (const char *attribute = element->Attribute("type")) {
        type = attribute;
    }
    return normalize_token(type);
}

bool is_light_element(tinyxml2::XMLElement *element) {
    const std::string type = light_type(element);
    return type == "light" || type == "directional" ||
           type == "directionallight" || type == "sun" || type == "point" ||
           type == "pointlight" || type == "spot" || type == "spotlight";
}

bool is_lights_container(tinyxml2::XMLElement *element) {
    const char *name = element->Name();
    return name != nullptr && normalize_token(name) == "lights";
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

Color parse_light_color(tinyxml2::XMLElement *element,
                        std::initializer_list<const char *> names) {
    return parse_vec3(first_attribute(element, names), Color{1.f});
}

glm::vec3 parse_light_position(tinyxml2::XMLElement *element,
                               const glm::vec3 &fallback = glm::vec3{0.f}) {
    return parse_vec3(first_attribute(element, {"position", "pos", "origin"}),
                      fallback);
}

glm::vec3 parse_light_direction(tinyxml2::XMLElement *element,
                                const glm::vec3 &fallback = glm::vec3{
                                    0.f, -1.f, 0.f}) {
    return parse_vec3(
        first_attribute(element, {"direction", "dir", "vector", "axis"}),
        fallback);
}

glm::vec3 parse_light_attenuation(tinyxml2::XMLElement *element) {
    return parse_vec3(
        first_attribute(
            element,
            {"attenuation", "attenuations", "falloff", "attenuationFactors"}),
        glm::vec3{0.f, 0.f, 1.f});
}

void add_light_to_scene(tinyxml2::XMLElement *element, Scene *scene) {
    std::string type = light_type(element);

    // A bare <light .../> can infer its kind from the supplied attributes.
    if (type == "light") {
        const bool has_position = element->Attribute("position") != nullptr ||
                                  element->Attribute("pos") != nullptr ||
                                  element->Attribute("origin") != nullptr;
        const bool has_direction = element->Attribute("direction") != nullptr ||
                                   element->Attribute("dir") != nullptr ||
                                   element->Attribute("vector") != nullptr;
        if (has_position && has_direction) {
            type = "spot";
        } else if (has_position) {
            type = "point";
        } else {
            type = "directional";
        }
    }

    if (type == "directional" || type == "directionallight" || type == "sun") {
        const glm::vec3 direction = parse_light_direction(element);
        const Color radiance = parse_light_color(
            element,
            {"color", "radiance", "intensity", "colour", "emission", "power"});
        scene->create_light<DirectionalLight>(direction, radiance);
    } else if (type == "point" || type == "pointlight") {
        const glm::vec3 position = parse_light_position(element);
        const Color intensity = parse_light_color(
            element,
            {"color", "intensity", "radiance", "colour", "emission", "power"});
        const glm::vec3 attenuation = parse_light_attenuation(element);
        scene->create_light<PointLight>(position, intensity, attenuation);
    } else if (type == "spot" || type == "spotlight") {
        const glm::vec3 position = parse_light_position(element);
        const glm::vec3 direction = parse_light_direction(element);
        const Color intensity = parse_light_color(
            element,
            {"color", "intensity", "radiance", "colour", "emission", "power"});
        const glm::vec3 attenuation = parse_light_attenuation(element);

        // XML 中的聚光灯角度使用角度制，SpotLight 构造函数使用弧度制。
        float inner_angle_degrees =
            parse_float(first_attribute(element,
                                        {"innerAngle",
                                         "inner_angle",
                                         "innerConeAngle",
                                         "innerCone",
                                         "inner",
                                         "angle"}),
                        20.f);
        float outer_angle_degrees =
            parse_float(first_attribute(element,
                                        {"outerAngle",
                                         "outer_angle",
                                         "outerConeAngle",
                                         "outerCone",
                                         "outer",
                                         "angle"}),
                        30.f);

        if (inner_angle_degrees >= outer_angle_degrees) {
            outer_angle_degrees = inner_angle_degrees + 1.f;
        }

        const float inner_angle = glm::radians(inner_angle_degrees);
        const float outer_angle = glm::radians(outer_angle_degrees);

        scene->create_light<SpotLight>(position,
                                       direction,
                                       intensity,
                                       inner_angle,
                                       outer_angle,
                                       attenuation);
    }
}

bool is_material_element(tinyxml2::XMLElement *element) {
    const char *name = element->Name();
    return name != nullptr && normalize_token(name) == "material";
}

bool is_materials_container(tinyxml2::XMLElement *element) {
    const char *name = element->Name();
    return name != nullptr && normalize_token(name) == "materials";
}

bool is_objects_container(tinyxml2::XMLElement *element) {
    const char *name = element->Name();
    return name != nullptr && normalize_token(name) == "objects";
}

bool is_object_element(tinyxml2::XMLElement *element) {
    const char *name = element->Name();
    return name != nullptr && normalize_token(name) == "object";
}

Color parse_material_albedo(tinyxml2::XMLElement *element) {
    return parse_vec3(first_attribute(element,
                                      {"albedo",
                                       "color",
                                       "colour",
                                       "diffuse",
                                       "baseColor",
                                       "base_color"}),
                      Color{0.8f});
}

Color parse_material_emissive(tinyxml2::XMLElement *element) {
    return parse_vec3(first_attribute(element,
                                      {"emissive",
                                       "emission",
                                       "emissiveColor",
                                       "emissive_color",
                                       "emit"}),
                      Color{0.f});
}

bool attribute_is_on(const char *value) {
    if (value == nullptr) {
        return false;
    }

    const std::string normalized = normalize_token(value);
    return normalized == "true" || normalized == "1" || normalized == "yes" ||
           normalized == "on" || normalized == "area";
}

bool is_area_light_object(tinyxml2::XMLElement *element) {
    return attribute_is_on(first_attribute(element,
                                           {"areaLight",
                                            "area_light",
                                            "area",
                                            "lightType",
                                            "light_type",
                                            "light",
                                            "type"}));
}

std::shared_ptr<Material>
parse_material_definition(tinyxml2::XMLElement *element, Scene *scene) {
    const char *name_attribute = first_attribute(element, {"name", "id"});
    const char *type_attribute = first_attribute(element, {"type", "model"});

    std::string type = "lambert";
    if (type_attribute != nullptr) {
        type = normalize_token(type_attribute);
    }

    std::string name;
    if (name_attribute != nullptr && name_attribute[0] != '\0') {
        name = name_attribute;
        if (auto existing = scene->get_materials(name)) {
            return existing;
        }
    }

    const bool is_dielectric = type == "dielectric" ||
                               type == "dielectricspecular" ||
                               type == "glass" || type == "insulator";
    if (is_dielectric) {
        const float eta_attribute = parse_float(
            first_attribute(
                element,
                {"eta", "ior", "indexOfRefraction", "index_of_refraction"}),
            1.5f);
        const float eta = eta_attribute > 0.f ? eta_attribute : 1.5f;

        const Color transmission_color =
            parse_vec3(first_attribute(element,
                                       {"transmissionColor",
                                        "transmission_color",
                                        "transmission",
                                        "color",
                                        "colour",
                                        "tint"}),
                       Color{1.f});
        const Color reflection_tint =
            parse_vec3(first_attribute(element,
                                       {"reflectionColor",
                                        "reflection_color",
                                        "reflectionTint",
                                        "reflection_tint",
                                        "reflectance",
                                        "reflectivity"}),
                       Color{1.f});
        const Color emissive = parse_material_emissive(element);

        if (!name.empty()) {
            return scene->create_material<DielectricSpecularMaterial>(
                name, eta, transmission_color, reflection_tint, emissive);
        }

        return std::make_shared<DielectricSpecularMaterial>(
            eta, transmission_color, reflection_tint, emissive);
    }

    const bool is_conductor = type == "conductor" ||
                              type == "conductorspecular" || type == "metal" ||
                              type == "specular" || type == "mirror";
    if (is_conductor) {
        const Color eta = parse_vec3(
            first_attribute(
                element,
                {"eta", "ior", "indexOfRefraction", "index_of_refraction"}),
            Color{0.f});
        const Color absorption =
            parse_vec3(first_attribute(element,
                                       {"absorption",
                                        "k",
                                        "absorptionCoeff",
                                        "absorptionCoefficient",
                                        "absorption_coefficient",
                                        "extinction"}),
                       Color{0.f});
        const Color reflection_color =
            parse_vec3(first_attribute(element,
                                       {"reflectionColor",
                                        "reflection_color",
                                        "reflectance",
                                        "reflectivity",
                                        "tint",
                                        "color",
                                        "colour",
                                        "albedo"}),
                       Color{1.f});
        const Color emissive = parse_material_emissive(element);

        if (!name.empty()) {
            return scene->create_material<ConductorSpecularMaterial>(
                name, eta, absorption, reflection_color, emissive);
        }

        return std::make_shared<ConductorSpecularMaterial>(
            eta, absorption, reflection_color, emissive);
    }

    const Color albedo = parse_material_albedo(element);
    const Color emissive = parse_material_emissive(element);

    if (!name.empty()) {
        return scene->create_material<LambertMaterial>(name, albedo, emissive);
    }

    return std::make_shared<LambertMaterial>(albedo, emissive);
}

std::shared_ptr<Material>
parse_scene_object_material(tinyxml2::XMLElement *element, Scene *scene) {
    // <object material="red">...</object> 或 <object materialRef="red">
    if (const char *reference = first_attribute(element,
                                                {"material",
                                                 "materialName",
                                                 "material_name",
                                                 "materialRef",
                                                 "material_ref"})) {
        if (auto material = scene->get_materials(reference)) {
            return material;
        }
    }

    // <object><material .../> ... </object>
    for (tinyxml2::XMLElement *child = element->FirstChildElement("material");
         child != nullptr;
         child = child->NextSiblingElement("material")) {
        return parse_material_definition(child, scene);
    }

    return nullptr;
}

bool is_primitive_element(tinyxml2::XMLElement *element) {
    const char *name = element->Name();
    if (name == nullptr) {
        return false;
    }

    const std::string type(name);
    return type == "triangle" || type == "sphere" || type == "disk";
}

glm::vec3 parse_scene_object_position(tinyxml2::XMLElement *element) {
    return parse_vec3(
        first_attribute(element, {"position", "center", "origin"}),
        glm::vec3{0.f});
}

glm::vec3 parse_scene_object_rotation(tinyxml2::XMLElement *element) {
    // XML 中的 rotation/euler 使用角度制，SceneObject 内部使用弧度制。
    const glm::vec3 rotation_degrees = parse_vec3(
        first_attribute(element, {"rotation", "euler"}), glm::vec3{0.f});
    return glm::radians(rotation_degrees);
}

glm::vec3 parse_scene_object_scale(tinyxml2::XMLElement *element) {
    return parse_vec3(element->Attribute("scale"), glm::vec3{1.f});
}

void add_scene_object_element(tinyxml2::XMLElement *element, Scene *scene) {
    const glm::vec3 position = parse_scene_object_position(element);
    const glm::vec3 rotation = parse_scene_object_rotation(element);
    const glm::vec3 scale = parse_scene_object_scale(element);

    auto scene_object = scene->create_scene_object(position, rotation, scale);

    if (auto material = parse_scene_object_material(element, scene)) {
        scene_object->set_material(material);
    }

    for (tinyxml2::XMLElement *primitive = element->FirstChildElement();
         primitive != nullptr;
         primitive = primitive->NextSiblingElement()) {
        if (is_material_element(primitive)) {
            continue;
        }

        add_primitive_to_scene_object(primitive, scene_object.get());
    }

    if (is_area_light_object(element)) {
        scene->create_light<AreaLight>(scene_object);
    }
}

void add_objects_container(tinyxml2::XMLElement *container, Scene *scene) {
    for (tinyxml2::XMLElement *object = container->FirstChildElement("object");
         object != nullptr;
         object = object->NextSiblingElement("object")) {
        add_scene_object_element(object, scene);
    }
}

} // namespace

std::unique_ptr<Scene>
SceneLoader::load_scene_from_xml(const std::string &scene_file, int w, int h) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(scene_file.c_str()) != tinyxml2::XML_SUCCESS) {
        return nullptr;
    }

    tinyxml2::XMLElement *root = doc.RootElement();
    if (root == nullptr) {
        return nullptr;
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
        // Camera FOV in XML is specified in degrees.
        fov_degrees = parse_float(camera->Attribute("fov"), 60.f);
        near_plane =
            parse_float(first_attribute(camera, {"near", "nearPlane"}), 0.1f);
        far_plane =
            parse_float(first_attribute(camera, {"far", "farPlane"}), 1000.f);
    }

    auto scene = std::make_unique<Scene>(camera_position,
                                         camera_target,
                                         camera_up,
                                         glm::radians(fov_degrees),
                                         near_plane,
                                         far_plane,
                                         w,
                                         h);

    // Named material definitions.  Collect them before parsing objects so
    // material references work regardless of declaration order.
    for (tinyxml2::XMLElement *element = root->FirstChildElement();
         element != nullptr;
         element = element->NextSiblingElement()) {
        if (is_material_element(element)) {
            parse_material_definition(element, scene.get());
        } else if (is_materials_container(element)) {
            for (tinyxml2::XMLElement *material = element->FirstChildElement();
                 material != nullptr;
                 material = material->NextSiblingElement()) {
                if (is_material_element(material)) {
                    parse_material_definition(material, scene.get());
                }
            }
        }
    }

    // Objects. Canonical scenes use <objects>; direct <object> is kept for
    // backward compatibility with older scene files.
    for (tinyxml2::XMLElement *element = root->FirstChildElement();
         element != nullptr;
         element = element->NextSiblingElement()) {
        if (is_objects_container(element)) {
            add_objects_container(element, scene.get());
        } else if (is_object_element(element)) {
            add_scene_object_element(element, scene.get());
        }
    }

    // Root-level primitives and lights.
    for (tinyxml2::XMLElement *element = root->FirstChildElement();
         element != nullptr;
         element = element->NextSiblingElement()) {
        if (is_lights_container(element)) {
            for (tinyxml2::XMLElement *light = element->FirstChildElement();
                 light != nullptr;
                 light = light->NextSiblingElement()) {
                if (is_light_element(light)) {
                    add_light_to_scene(light, scene.get());
                }
            }
            continue;
        }

        if (is_light_element(element)) {
            add_light_to_scene(element, scene.get());
            continue;
        }

        if (!is_primitive_element(element)) {
            continue;
        }

        const glm::vec3 position = parse_scene_object_position(element);
        const glm::vec3 rotation = parse_scene_object_rotation(element);
        const glm::vec3 scale = parse_scene_object_scale(element);

        auto scene_object =
            scene->create_scene_object(position, rotation, scale);

        if (auto material = parse_scene_object_material(element, scene.get())) {
            scene_object->set_material(material);
        }

        add_primitive_to_scene_object(element, scene_object.get());
    }

    return scene;
}

std::unique_ptr<Scene> SceneLoader::create_default_scene(int w, int h) {
    auto scene = std::make_unique<Scene>(glm::vec3{0.f, 0.f, 0.f},
                                         glm::vec3{0.f, 0.f, 1000.f},
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

    scene->create_light<PointLight>(glm::vec3{4.f, 4.f, 3.f},
                                    Color{1.f, 0.95f, 0.9f},
                                    glm::vec3{0.f, 0.f, 1.f});
    scene->create_light<DirectionalLight>(glm::vec3{-0.4f, -1.f, -0.5f},
                                          Color{0.25f});

    return scene;
}
