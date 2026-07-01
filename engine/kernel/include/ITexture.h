#pragma once

#include "types.h"

namespace arch::core::renderer {

enum class TextureFormat { RGB, RGBA, Depth, Stencil, R8 };

enum class TextureWrap { Repeat, Clamp, Mirror };

class ITexture {
public:
  virtual ~ITexture() = default;

  virtual void Bind(u32 slot = 0) = 0;
  virtual void Unbind() const = 0;

  virtual u32 GetWidth() const = 0;
  virtual u32 GetHeight() const = 0;
  virtual TextureFormat GetFormat() const = 0;
};
} // namespace arch::core::renderer
