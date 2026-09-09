#include "render.h"

#include <string>

int main(int argc, char *argv[]) {
    const std::string scene_file = argc > 1 ? argv[1] : "scenes/scenes.xml";
    Render render{800, 600, 50, scene_file};
    render.run();

    return 0;
}
