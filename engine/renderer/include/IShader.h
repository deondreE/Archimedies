#pragma once

namespace arch::core::renderer {
enum class ShaderStage { Vertex, Fragment, Compute };

class IShader {
public:
  virtual ~IShader() = default;
  virtual void Bind() = 0;
};
} // namespace arch::core::renderer