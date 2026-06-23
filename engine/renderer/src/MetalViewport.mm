#include "UiElement.h"
#import <Metal/Metal.h>

namespace arch::core::ui {

class MetalViewport : public IViewport 
{
public: 
    MetalViewport(id<MTLDevice> device, std::string title)
        : _title(std::move(title)), _device(device)
    {
        _bounds = {0, 0, 800, 600};
        RecreateTexture();
    }


    void SetBounds(Rect bounds) override 
    {
        if (_bounds.w != bounds.w || _bounds.h != bounds.h)
        {
            _bounds = bounds;
            RecreateTexture();
        } else {
            _bounds = bounds;
        }
    }

    Rect GetBounds() const override { return _bounds; }
    void SetTitle(const std::string& title) override { _title = title; }
    std::string GetTitle() const override { return _title; }

    void* GetNativeRenderTarget() override {
        return (__bridge void*)_renderTarget;
    }

    void OnUpdate(float dt) override {

    }

    bool OnMouseMoved(float x, float y) override {
        _isHovered = _bounds.Contains(x, y);
        return _isHovered;
    }
private:
    void RecreateTexture() {
        if (_bounds.w <= 0 || _bounds.h <= 0) return;

        MTLTextureDescriptor* desc = [MTLTextureDescriptor 
            texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                         width:(NSUInteger)_bounds.w
                                        height:(NSUInteger)_bounds.h
                                     mipmapped:NO];
        desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        desc.storageMode = MTLStorageModePrivate;

        _renderTarget = [_device newTextureWithDescriptor:desc];
    }

    std::string _title;
    Rect _bounds;
    bool _isHovered = false;
    id<MTLDevice> _device;
    id<MTLTexture> _renderTarget;
};

}