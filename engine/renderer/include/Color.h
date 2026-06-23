#pragma once

#include "types.h"

namespace arch::core::renderer {
struct Color {
  float r, g, b, a;

  static constexpr Color FromRGB(u8 r, u8 g, u8 b, u8 a = 255) {
    return {static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f, static_cast<float>(a) / 255.0f};
  }
};
} // namespace  arch::core::renderer
