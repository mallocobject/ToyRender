#include "renderer.h"

#include <string>

int main(int argc, char *argv[]) {
    const std::string scene_file = argc > 1 ? argv[1] : "scenes/scene02.xml";
    Renderer renderer{800, 600, 1000, 3, 15, scene_file};
    renderer.run();

    return 0;
}
