#pragma once
#include "Entity.h"

#include <string_view>
#include <vector>

namespace arch::core::kernel {

enum class SceneType : u8 { _3d = 0, _2d, Headless };

struct Scene {
  std::string_view name;
  SceneType type;
  std::vector<Entity> entities;
};
} // namespace arch::core::kernel
