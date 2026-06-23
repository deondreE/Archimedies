#pragma once

#include "Color.h"
#include "IShader.h"
#include "ITexture.h"
#include "types.h"
#include <memory>
#include <type_traits>
#include <vector>

namespace arch::core::renderer {
struct RendererConfig {
  void *WindowHandle = nullptr;
  u32 width{};
  u32 height{};
  bool vsync{};
};
struct TextureConfig {
  u32 width;
  u32 height;
  TextureFormat format;
  void *data = nullptr;
};

class MetalRenderer;

class IRenderer {
public:
  virtual ~IRenderer() = default;
  virtual bool Initialize(const RendererConfig &config) = 0;
  virtual void Shutdown() = 0;

  virtual void BeginFrame() = 0;
  virtual void EndFrame() = 0;

  virtual void Clear(const Color &color) = 0;

  virtual void OnResize(u32 width, u32 height) = 0;

  virtual std::shared_ptr<IShader>
  CreateShader(const std::vector<u32> &spirvBinary, ShaderStage stage) = 0;
  virtual void BindShader(std::shared_ptr<IShader> shader) = 0;
  virtual std::shared_ptr<ITexture>
  CreateTexture(const TextureConfig &config) = 0;
  virtual void Draw() = 0;

  virtual void *GetCurrentCommandBuffer() = 0;
  virtual void *GetRenderPassDescriptor() = 0;
  virtual void *GetDevice() = 0;

  static std::unique_ptr<IRenderer> Create();
};
} // namespace arch::core::renderer