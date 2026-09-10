#pragma once

#include "scene.h"

#include <memory>
#include <string>

class SceneLoader {
  public:
    // Load a scene from an XML file.  Returns nullptr when the file cannot
    // be opened/parsed; callers can then fall back to create_default_scene().
    static std::unique_ptr<Scene>
    load_scene_from_xml(const std::string &scene_file, int w, int h);

    // Convenience aliases.
    static std::unique_ptr<Scene> load(const std::string &scene_file,
                                       int w,
                                       int h) {
        return load_scene_from_xml(scene_file, w, h);
    }

    static std::unique_ptr<Scene> load_scene(const std::string &scene_file,
                                             int w,
                                             int h) {
        return load_scene_from_xml(scene_file, w, h);
    }

    // Create a small built-in scene containing geometry and a light source.
    static std::unique_ptr<Scene> create_default_scene(int w, int h);
};
