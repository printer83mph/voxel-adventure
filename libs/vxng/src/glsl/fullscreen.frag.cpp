#include "shaders.h"

namespace vxng::shaders {

const std::string FULLSCREEN_FRAG = R"(
#version 410 core

in vec2 texcoords;
out vec4 FragColor;

void main() {
    FragColor = vec4(texcoords.x, texcoords.y, 1.0, 1.0);
}
)";

} // namespace vxng::shaders
