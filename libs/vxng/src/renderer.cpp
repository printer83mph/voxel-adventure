#include "vxng/renderer.h"

namespace vxng {

Renderer::Renderer() {};
Renderer::~Renderer() {};

auto Renderer::set_scene(const vxng::Scene *scene) -> void {};
auto Renderer::render() const -> void {};

} // namespace vxng
