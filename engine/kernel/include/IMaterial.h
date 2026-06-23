#pragma once

#include "ITexture.h"
#include "Maths.h"
#include "Ref.h"

namespace arch::core::renderer {
using namespace arch::core::math;

/**
 * Materials act as the bridge between Shaders and Data (Textures/Uniforms).
 */
class IMaterial {
public:
  virtual ~IMaterial() = default;

  virtual void Bind() = 0;

  // Uniform Setters (High performance kernel path)
  virtual void SetFloat(const std::string &name, float value) = 0;
  virtual void SetVector3(const std::string &name, const Vector3 &value) = 0;
  virtual void SetMatrix4(const std::string &name, const Matrix4 &value) = 0;
  virtual void SetTexture(const std::string &name,
                          kernel::Ref<ITexture> texture) = 0;

  virtual const std::string &GetName() const = 0;
};
} // namespace arch::core::renderer