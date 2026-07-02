#pragma once

#include "Color.h"
#include "IShader.h"
#include "ITexture.h"
#include "types.h"
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#if defined(__APPLE__)
#include <simd/simd.h>
#endif

namespace arch::core::renderer {

#if defined(__APPLE__)

// Per-vertex data sent to the text shader.
struct TextVertex {
  simd_float2 position;
  simd_float2 texCoord;
  simd_float4 color;
};

// Glyph metrics stored in the font atlas.
struct Character {
  simd_float2 size;
  simd_float2 bearing;
  float next;
  simd_float2 uvTopLeft;
  simd_float2 uvBottomRight;
};

// Inline MSL source for text rendering.
static const char *textShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float2 position;
    float2 texCoord;
    float4 color;
};

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
    float4 color;
};

vertex VertexOut vertex_text(constant VertexIn*  verts      [[buffer(0)]],
                             constant float4x4&  projection [[buffer(1)]],
                             uint                vid        [[vertex_id]]) {
    VertexOut out;
    out.position = projection * float4(verts[vid].position, 0.0, 1.0);
    out.texCoord = verts[vid].texCoord;
    out.color    = verts[vid].color;
    return out;
}

fragment float4 fragment_text(VertexOut         in    [[stage_in]],
                              texture2d<float>  atlas [[texture(0)]]) {
    constexpr sampler s(filter::linear, address::clamp_to_edge);
    float a = atlas.sample(s, in.texCoord).r;
    return float4(in.color.rgb, in.color.a * a);
}
)";

#endif // __APPLE__

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

  // Draw a UTF-8 string at screen-space position (x, y) with the given
  // scale and RGBA color.  Must be called between BeginFrame/EndFrame.
  virtual void DrawText(const std::string &text, float x, float y, float scale,
                        float r, float g, float b, float a = 1.0f) = 0;

  virtual void *GetCurrentCommandBuffer() = 0;
  virtual void *GetRenderPassDescriptor() = 0;
  virtual void *GetDevice() = 0;

  static std::unique_ptr<IRenderer> Create();
};

} // namespace arch::core::renderer
