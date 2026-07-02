#include "IRenderer.h"
#include "ITexture.h"
#include <Cocoa/Cocoa.h>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <memory>
#include <spirv_msl.hpp>
#include <string>
#include <vector>

static constexpr const char *kDefaultFontPath =
    "/System/Library/Fonts/Monaco.ttf";
static constexpr uint32_t kAtlasFontSize = 48;
static constexpr uint32_t kAtlasSize = 1024;

namespace arch::core::renderer {

// ---------------------------------------------------------------------------
// MetalShader
// ---------------------------------------------------------------------------
class MetalShader : public IShader {
public:
  id<MTLFunction> function = nil;
  ShaderStage stage;

  MetalShader(id<MTLFunction> func, ShaderStage s) : function(func), stage(s) {}
  void Bind() override {}
};

// ---------------------------------------------------------------------------
// MetalTexture
// ---------------------------------------------------------------------------
class MetalTexture : public ITexture {
public:
  id<MTLTexture> texture = nil;
  TextureFormat format;
  u32 width, height;

  MetalTexture(id<MTLTexture> tex, TextureFormat fmt, u32 w, u32 h)
      : texture(tex), format(fmt), width(w), height(h) {}

  void Bind(u32 /*slot*/ = 0) override {}
  void Unbind() const override {}
  [[nodiscard]] u32 GetWidth() const override { return width; }
  [[nodiscard]] u32 GetHeight() const override { return height; }
  [[nodiscard]] TextureFormat GetFormat() const override { return format; }
};

// ---------------------------------------------------------------------------
// MetalFont  –  FreeType glyph atlas baked into a Metal R8 texture
// ---------------------------------------------------------------------------
class MetalFont {
public:
  std::map<char, Character> characters;
  std::shared_ptr<MetalTexture> texture;

  static std::shared_ptr<MetalFont> Load(const std::string &path, uint32_t size,
                                         id<MTLDevice> device) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
      return nullptr;

    FT_Face face;
    if (FT_New_Face(ft, path.c_str(), 0, &face)) {
      FT_Done_FreeType(ft);
      return nullptr;
    }
    FT_Set_Pixel_Sizes(face, 0, size);

    const uint32_t atlasW = kAtlasSize;
    const uint32_t atlasH = kAtlasSize;
    std::vector<uint8_t> pixels(atlasW * atlasH, 0);

    uint32_t penX = 0, penY = 0, rowH = 0;
    auto font = std::make_shared<MetalFont>();

    for (unsigned char c = 32; c < 127; c++) { // printable ASCII only
      if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        continue;

      FT_GlyphSlot g = face->glyph;
      uint32_t gW = g->bitmap.width;
      uint32_t gH = g->bitmap.rows;

      // 2-pixel padding between glyphs avoids bleed when sampling.
      constexpr uint32_t pad = 2;
      if (penX + gW + pad > atlasW) {
        penX = 0;
        penY += rowH + pad;
        rowH = 0;
      }
      if (penY + gH > atlasH)
        break;

      for (uint32_t row = 0; row < gH; ++row)
        for (uint32_t col = 0; col < gW; ++col)
          pixels[(penY + row) * atlasW + (penX + col)] =
              g->bitmap.buffer[row * g->bitmap.pitch + col];

      font->characters[static_cast<char>(c)] = {
          {static_cast<float>(gW), static_cast<float>(gH)},
          {static_cast<float>(g->bitmap_left),
           static_cast<float>(g->bitmap_top)},
          static_cast<float>(g->advance.x >> 6),
          {static_cast<float>(penX) / atlasW,
           static_cast<float>(penY) / atlasH},
          {static_cast<float>(penX + gW) / atlasW,
           static_cast<float>(penY + gH) / atlasH}};

      penX += gW + pad;
      rowH = std::max(rowH, gH);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    MTLTextureDescriptor *desc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
                                     width:atlasW
                                    height:atlasH
                                 mipmapped:NO];
    desc.usage = MTLTextureUsageShaderRead;

    id<MTLTexture> tex = [device newTextureWithDescriptor:desc];
    [tex replaceRegion:MTLRegionMake2D(0, 0, atlasW, atlasH)
           mipmapLevel:0
             withBytes:pixels.data()
           bytesPerRow:atlasW];

    font->texture =
        std::make_shared<MetalTexture>(tex, TextureFormat::R8, atlasW, atlasH);
    return font;
  }
};

// ---------------------------------------------------------------------------
// MetalRenderer
// ---------------------------------------------------------------------------
class MetalRenderer : public IRenderer {
public:
  MetalRenderer() = default;

  // -------------------------------------------------------------------------
  bool Initialize(const RendererConfig &config) override {
    _device = MTLCreateSystemDefaultDevice();
    if (!_device)
      return false;

    _commandQueue = [_device newCommandQueue];

    auto *window = (__bridge NSWindow *)config.WindowHandle;
    _metalLayer = [CAMetalLayer layer];
    _metalLayer.device = _device;
    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalLayer.framebufferOnly = YES;
    _metalLayer.frame = window.contentView.frame;

    // Match the physical pixel density so text is crisp on Retina displays.
    _metalLayer.contentsScale = window.backingScaleFactor;
    _metalLayer.drawableSize = CGSizeMake(
        window.contentView.frame.size.width * window.backingScaleFactor,
        window.contentView.frame.size.height * window.backingScaleFactor);

    _renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    [window.contentView setWantsLayer:YES];
    [window.contentView setLayer:_metalLayer];

    InitializeTextPipeline();
    _defaultFont = MetalFont::Load(kDefaultFontPath, kAtlasFontSize, _device);

    return true;
  }

  // -------------------------------------------------------------------------
  void BeginFrame() override {
    _currentDrawable = [_metalLayer nextDrawable];
    _currentCommandBuffer = [_commandQueue commandBuffer];
    _textBatch.clear();
  }

  // -------------------------------------------------------------------------
  // Store the clear color; the actual clear happens at the start of EndFrame
  // so it is always the very first load action on the drawable.
  void Clear(const Color &color) override {
    _clearColor = MTLClearColorMake(color.r, color.g, color.b, color.a);
    _hasClear = true;
  }

  // -------------------------------------------------------------------------
  // Flush all accumulated draw calls in a single command buffer commit.
  void EndFrame() override {
    if (!_currentDrawable)
      return;

    // --- 1. Clear pass (or load if no Clear() was called) ----------------
    _renderPassDescriptor.colorAttachments[0].texture =
        _currentDrawable.texture;
    _renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    _renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    _renderPassDescriptor.colorAttachments[0].clearColor =
        _hasClear ? _clearColor : MTLClearColorMake(0, 0, 0, 1);
    _hasClear = false;

    // Open one encoder for the clear (and any geometry Draw() queued).
    id<MTLRenderCommandEncoder> enc = [_currentCommandBuffer
        renderCommandEncoderWithDescriptor:_renderPassDescriptor];

    if (_pipelineState && _pendingDraw) {
      [enc setRenderPipelineState:_pipelineState];
      [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
      _pendingDraw = false;
    }
    [enc endEncoding];

    // --- 2. Single text pass for the entire batch ------------------------
    if (!_textBatch.empty() && _textPSO && _defaultFont) {
      // Compute ortho projection from current drawable size.
      float L = 0.0f, R = static_cast<float>(_metalLayer.drawableSize.width);
      float T = 0.0f, B = static_cast<float>(_metalLayer.drawableSize.height);
      simd_float4x4 ortho = simd_matrix_from_rows(
          simd_make_float4(2.0f / (R - L), 0.0f, 0.0f, -(R + L) / (R - L)),
          simd_make_float4(0.0f, 2.0f / (B - T), 0.0f, -(B + T) / (B - T)),
          simd_make_float4(0.0f, 0.0f, 0.5f, 0.5f),
          simd_make_float4(0.0f, 0.0f, 0.0f, 1.0f));

      // Load (don't clear) — composites over whatever was just drawn.
      _renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;
      _renderPassDescriptor.colorAttachments[0].storeAction =
          MTLStoreActionStore;

      id<MTLRenderCommandEncoder> textEnc = [_currentCommandBuffer
          renderCommandEncoderWithDescriptor:_renderPassDescriptor];
      [textEnc setRenderPipelineState:_textPSO];
      [textEnc setVertexBytes:_textBatch.data()
                       length:sizeof(TextVertex) * _textBatch.size()
                      atIndex:0];
      [textEnc setVertexBytes:&ortho length:sizeof(ortho) atIndex:1];
      [textEnc setFragmentTexture:_defaultFont->texture->texture atIndex:0];
      [textEnc drawPrimitives:MTLPrimitiveTypeTriangle
                  vertexStart:0
                  vertexCount:static_cast<NSUInteger>(_textBatch.size())];
      [textEnc endEncoding];

      _textBatch.clear();
    }

    [_currentCommandBuffer presentDrawable:_currentDrawable];
    [_currentCommandBuffer commit];

    _currentDrawable = nil;
    _currentCommandBuffer = nil;
  }

  // -------------------------------------------------------------------------
  void Shutdown() override {
    _defaultFont.reset();
    _commandQueue = nil;
    _device = nil;
    _metalLayer = nil;
  }

  // -------------------------------------------------------------------------
  void OnResize(uint32_t width, uint32_t height) override {
    _metalLayer.drawableSize = CGSizeMake(width, height);
  }

  // -------------------------------------------------------------------------
  std::shared_ptr<IShader> CreateShader(const std::vector<u32> &spirvBinary,
                                        ShaderStage stage) override {
    spirv_cross::CompilerMSL mslCompiler(spirvBinary);
    spirv_cross::CompilerMSL::Options options;
    options.set_msl_version(2, 3);
    options.platform = spirv_cross::CompilerMSL::Options::macOS;
    mslCompiler.set_msl_options(options);

    std::string mslSource = mslCompiler.compile();
    NSString *source = [NSString stringWithUTF8String:mslSource.c_str()];

    NSError *error = nil;
    id<MTLLibrary> library =
        [_device newLibraryWithSource:source
                              options:[MTLCompileOptions new]
                                error:&error];
    if (!library)
      return nullptr;

    id<MTLFunction> function = [library newFunctionWithName:@"main0"];
    if (!function)
      return nullptr;

    return std::make_shared<MetalShader>(function, stage);
  }

  // -------------------------------------------------------------------------
  void BindShader(std::shared_ptr<IShader> shader) override {
    if (!shader)
      return;

    auto mShader = std::static_pointer_cast<MetalShader>(shader);
    bool changed = false;

    if (mShader->stage == ShaderStage::Vertex) {
      if (_currentVert != mShader) {
        _currentVert = mShader;
        changed = true;
      }
    } else if (mShader->stage == ShaderStage::Fragment) {
      if (_currentFrag != mShader) {
        _currentFrag = mShader;
        changed = true;
      }
    }

    if (changed && _currentVert && _currentFrag)
      BuildPipelineState();
  }

  // -------------------------------------------------------------------------
  // Queue a geometry draw; the actual GPU work happens in EndFrame.
  void Draw() override {
    if (_pipelineState)
      _pendingDraw = true;
  }

  // -------------------------------------------------------------------------
  std::shared_ptr<ITexture>
  CreateTexture(const TextureConfig &config) override {
    MTLTextureDescriptor *desc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                     width:config.width
                                    height:config.height
                                 mipmapped:NO];
    desc.usage = MTLTextureUsageShaderRead;

    id<MTLTexture> tex = [_device newTextureWithDescriptor:desc];
    if (config.data) {
      [tex replaceRegion:MTLRegionMake2D(0, 0, config.width, config.height)
             mipmapLevel:0
               withBytes:config.data
             bytesPerRow:config.width * 4];
    }

    return std::make_shared<MetalTexture>(tex, config.format, config.width,
                                          config.height);
  }

  // -------------------------------------------------------------------------
  // Accumulate text vertices into the batch; GPU work happens in EndFrame.
  void DrawText(const std::string &text, float x, float y, float scale, float r,
                float g, float b, float a = 1.0f) override {
    if (!_defaultFont)
      return;

    // The atlas was baked at kAtlasFontSize px; adjust scale so that passing
    // scale=1.0 renders at a readable logical size (24 px) on screen.
    constexpr float kLogicalSize = 24.0f;
    const float atlasScale = (kLogicalSize / kAtlasFontSize) * scale;

    // On a Retina display the drawable is 2× the window's point size, so we
    // also need to account for the backing scale factor.
    const float dpiScale = static_cast<float>(_metalLayer.contentsScale);

    const float finalScale = atlasScale * dpiScale;

    simd_float4 color = {r, g, b, a};
    float penX = x * dpiScale;
    float penY = y * dpiScale;

    for (char c : text) {
      auto it = _defaultFont->characters.find(c);
      if (it == _defaultFont->characters.end()) {
        // Advance by a space width for unknown characters.
        auto sp = _defaultFont->characters.find(' ');
        if (sp != _defaultFont->characters.end())
          penX += sp->second.next * finalScale;
        continue;
      }

      const Character &ch = it->second;
      float xpos = penX + ch.bearing.x * finalScale;
      float ypos = penY - (ch.size.y - ch.bearing.y) * finalScale;
      float w = ch.size.x * finalScale;
      float h = ch.size.y * finalScale;

      // Two CCW triangles forming the glyph quad.
      _textBatch.push_back(
          {{xpos, ypos + h}, {ch.uvTopLeft.x, ch.uvTopLeft.y}, color});
      _textBatch.push_back(
          {{xpos, ypos}, {ch.uvTopLeft.x, ch.uvBottomRight.y}, color});
      _textBatch.push_back(
          {{xpos + w, ypos}, {ch.uvBottomRight.x, ch.uvBottomRight.y}, color});
      _textBatch.push_back(
          {{xpos, ypos + h}, {ch.uvTopLeft.x, ch.uvTopLeft.y}, color});
      _textBatch.push_back(
          {{xpos + w, ypos}, {ch.uvBottomRight.x, ch.uvBottomRight.y}, color});
      _textBatch.push_back(
          {{xpos + w, ypos + h}, {ch.uvBottomRight.x, ch.uvTopLeft.y}, color});

      penX += ch.next * finalScale;
    }
  }

  // -------------------------------------------------------------------------
  void *GetCurrentCommandBuffer() override {
    return (__bridge void *)_currentCommandBuffer;
  }

  void *GetRenderPassDescriptor() override {
    _renderPassDescriptor.colorAttachments[0].texture =
        _currentDrawable.texture;
    return (__bridge void *)_renderPassDescriptor;
  }

  void *GetDevice() override { return (__bridge void *)_device; }

private:
  // -------------------------------------------------------------------------
  void BuildPipelineState() {
    MTLRenderPipelineDescriptor *desc = [MTLRenderPipelineDescriptor new];
    desc.vertexFunction = _currentVert->function;
    desc.fragmentFunction = _currentFrag->function;
    desc.colorAttachments[0].pixelFormat = _metalLayer.pixelFormat;
    desc.rasterSampleCount = 1;

    NSError *error = nil;
    id<MTLRenderPipelineState> state =
        [_device newRenderPipelineStateWithDescriptor:desc error:&error];
    if (state)
      _pipelineState = state;
  }

  // -------------------------------------------------------------------------
  void InitializeTextPipeline() {
    NSError *error = nil;
    NSString *source = [NSString stringWithUTF8String:textShaderSource];
    id<MTLLibrary> lib = [_device newLibraryWithSource:source
                                               options:nil
                                                 error:&error];
    if (!lib)
      return;

    MTLRenderPipelineDescriptor *desc = [MTLRenderPipelineDescriptor new];
    desc.vertexFunction = [lib newFunctionWithName:@"vertex_text"];
    desc.fragmentFunction = [lib newFunctionWithName:@"fragment_text"];
    desc.colorAttachments[0].pixelFormat = _metalLayer.pixelFormat;

    // Alpha blending — must set on the descriptor's attachment directly,
    // not on a local copy (auto ca = … would copy, not reference).
    desc.colorAttachments[0].blendingEnabled = YES;
    desc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    desc.colorAttachments[0].destinationRGBBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;
    desc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
    desc.colorAttachments[0].destinationAlphaBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;

    _textPSO = [_device newRenderPipelineStateWithDescriptor:desc error:&error];
  }

  // -------------------------------------------------------------------------
  id<MTLDevice> _device = nil;
  id<MTLCommandQueue> _commandQueue = nil;
  id<MTLCommandBuffer> _currentCommandBuffer = nil;
  id<CAMetalDrawable> _currentDrawable = nil;
  id<MTLRenderPipelineState> _pipelineState = nil;
  id<MTLRenderPipelineState> _textPSO = nil;
  MTLRenderPassDescriptor *_renderPassDescriptor = nil;
  CAMetalLayer *_metalLayer = nil;

  std::shared_ptr<MetalShader> _currentVert;
  std::shared_ptr<MetalShader> _currentFrag;
  std::shared_ptr<MetalFont> _defaultFont;

  // Per-frame batch of text vertices accumulated by DrawText().
  std::vector<TextVertex> _textBatch;

  MTLClearColor _clearColor = MTLClearColorMake(0, 0, 0, 1);
  bool _hasClear = false;
  bool _pendingDraw = false;
};

// ---------------------------------------------------------------------------
std::unique_ptr<IRenderer> IRenderer::Create() {
  return std::make_unique<MetalRenderer>();
}

} // namespace arch::core::renderer
