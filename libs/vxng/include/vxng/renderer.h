#pragma once

#include "vxng/scene.h"

namespace vxng {

class Renderer {
  public:
    Renderer();
    ~Renderer();
    auto set_scene(vxng::Scene const *scene) -> void;
    auto render() const -> void;

  private:
};

} // namespace vxng
