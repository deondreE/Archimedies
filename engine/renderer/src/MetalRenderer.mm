#include "IRenderer.h"
#include "ITexture.h"
#include <Cocoa/Cocoa.h>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <memory>
#include <spirv_msl.hpp>

namespace arch::core::renderer {
class MetalShader : public IShader {
public:
  id<MTLFunction> function = nil;
  ShaderStage stage;

  MetalShader(id<MTLFunction> func, ShaderStage s) : function(func), stage(s) {}

  void Bind() override {}
};

class MetalTexture : public ITexture {
public:
  id<MTLTexture> texture = nil;
  TextureFormat format;
  u32 width, height;

  MetalTexture(id<MTLTexture> tex, TextureFormat fmt, u32 w, u32 h)
      : texture(tex), format(fmt), width(w), height(h) {}

  void Bind(u32 slot = 0) override {}
  void Unbind() const override {}
  [[nodiscard]] u32 GetWidth() const override { return width; }
  [[nodiscard]] u32 GetHeight() const override { return height; }
  [[nodiscard]] TextureFormat GetFormat() const override { return format; }
};

class MetalRenderer : public IRenderer {
public:
  MetalRenderer() = default;

  bool Initialize(const RendererConfig &config) override {
    _device = MTLCreateSystemDefaultDevice();
    if (!_device)
      return false;

    _commandQueue = [_device newCommandQueue];

    auto window = (__bridge NSWindow *)config.WindowHandle;
    _metalLayer = [CAMetalLayer layer];
    _metalLayer.device = _device;
    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalLayer.framebufferOnly = YES;
    _metalLayer.frame = window.contentView.frame;

    _renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    [window.contentView setWantsLayer:YES];
    [window.contentView setLayer:_metalLayer];

    return _device != nil;
  }

  void BeginFrame() override {
    _currentDrawable = [_metalLayer nextDrawable];
    _currentCommandBuffer = [_commandQueue commandBuffer];
  }

  void Clear(const Color &color) override {
    if (!_currentDrawable)
      return;

    auto colorAttacment = _renderPassDescriptor.colorAttachments[0];
    colorAttacment.texture = _currentDrawable.texture;
    colorAttacment.loadAction = MTLLoadActionClear;
    colorAttacment.storeAction = MTLStoreActionStore;
    colorAttacment.clearColor =
        MTLClearColorMake(color.r, color.g, color.b, color.a);

    id<MTLRenderCommandEncoder> encoder = [_currentCommandBuffer
        renderCommandEncoderWithDescriptor:_renderPassDescriptor];
    [encoder endEncoding];
  }

  void EndFrame() override {
    if (!_currentDrawable)
      return;

    [_currentCommandBuffer presentDrawable:_currentDrawable];
    [_currentCommandBuffer commit];

    _currentDrawable = nil;
    _currentCommandBuffer = nil;
  }

  void Shutdown() override {
    _commandQueue = nil;
    _device = nil;
    _metalLayer = nil;
  }

  void OnResize(uint32_t width, uint32_t height) override {
    _metalLayer.drawableSize = CGSizeMake(width, height);
  }

  void BuildPipelineState() {
    MTLRenderPipelineDescriptor *desc = [MTLRenderPipelineDescriptor new];
    desc.vertexFunction = _currentVert->function;
    desc.fragmentFunction = _currentFrag->function;
    desc.colorAttachments[0].pixelFormat = _metalLayer.pixelFormat;
    desc.rasterSampleCount = 1;

    NSError *error = nil;
    id<MTLRenderPipelineState> newState =
        [_device newRenderPipelineStateWithDescriptor:desc error:&error];

    if (!newState) {
      return;
    }

    _pipelineState = newState;
  }

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

    if (changed && _currentVert && _currentFrag) {
      BuildPipelineState();
    }
  }

  std::shared_ptr<IShader> CreateShader(const std::vector<u32> &spirvBinary,
                                        ShaderStage stage) override {
    spirv_cross::CompilerMSL mslCompiler(spirvBinary);
    spirv_cross::CompilerMSL::Options options;
    options.set_msl_version(2, 3);
    options.platform = spirv_cross::CompilerMSL::Options::macOS;
    mslCompiler.set_msl_options(options);

    std::string mslSource = mslCompiler.compile();
    NSString *source = [NSString stringWithUTF8String:mslSource.c_str()];
    MTLCompileOptions *compileOptions = [MTLCompileOptions new];

    NSError *error = nil;
    id<MTLLibrary> library = [_device newLibraryWithSource:source
                                                   options:compileOptions
                                                     error:&error];
    if (!library)
      return nullptr;

    id<MTLFunction> function = [library newFunctionWithName:@"main0"];
    if (!function)
      return nullptr;

    return std::make_shared<MetalShader>(function, stage);
  }

  void Draw() override {
    if (!_currentDrawable || !_pipelineState)
      return;

    MTLRenderPassDescriptor *passDescriptor =
        [MTLRenderPassDescriptor renderPassDescriptor];
    passDescriptor.colorAttachments[0].texture = _currentDrawable.texture;
    passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDescriptor.colorAttachments[0].clearColor =
        MTLClearColorMake(0.1, 0.1, 0.1, 1.0);

    id<MTLRenderCommandEncoder> encoder = [_currentCommandBuffer
        renderCommandEncoderWithDescriptor:passDescriptor];
    [encoder setRenderPipelineState:_pipelineState];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                vertexStart:0
                vertexCount:3];
    [encoder endEncoding];
  }

  std::shared_ptr<ITexture>
  CreateTexture(const TextureConfig &config) override {
    MTLPixelFormat metalFormat = MTLPixelFormatRGBA8Unorm;
    u32 bytesPerPixel = 4;

    MTLTextureDescriptor *desc =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:metalFormat
                                                           width:config.width
                                                          height:config.height
                                                       mipmapped:NO];
    desc.usage = MTLTextureUsageShaderRead;

    id<MTLTexture> tex = [_device newTextureWithDescriptor:desc];

    if (config.data) {
      MTLRegion region = MTLRegionMake2D(0, 0, config.width, config.height);
      [tex replaceRegion:region
             mipmapLevel:0
               withBytes:config.data
             bytesPerRow:config.width * bytesPerPixel];
    }

    return std::make_shared<MetalTexture>(tex, config.format, config.width,
                                          config.height);
  }

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
  id<MTLDevice> _device = nil;
  id<MTLCommandQueue> _commandQueue = nil;
  id<MTLCommandBuffer> _currentCommandBuffer = nil;
  id<CAMetalDrawable> _currentDrawable = nil;
  id<MTLRenderPipelineState> _pipelineState = nil;
  MTLRenderPassDescriptor *_renderPassDescriptor = nil;
  CAMetalLayer *_metalLayer = nil;
  std::shared_ptr<MetalShader> _currentVert;
  std::shared_ptr<MetalShader> _currentFrag;
};

std::unique_ptr<IRenderer> IRenderer::Create() {
  return std::make_unique<MetalRenderer>();
}
} // namespace arch::core::renderer