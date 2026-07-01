#include "Application.h"
#include "Entity.h"
#include "IRenderer.h"
#include "ITexture.h"
#include "Scene.h"
#include "Serializer.h"
#include <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#include <Metal/Metal.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace arch::core::platform {
Application::Application(ApplicationConfig config)
    : _config(std::move(config)) {
  _renderer = renderer::IRenderer::Create();
  _meshLoader = std::make_unique<kernel::MeshLoader>();
}

Application::~Application() {
  if (engineThread.joinable()) {
    engineThread.join();
  }
}

void Application::Run() {
  kernel::Entity ent("test");
  std::unique_ptr<kernel::Scene> scene = std::make_unique<kernel::Scene>();
  scene->type = kernel::SceneType::_2d;
  scene->name = "test_scene";
  scene->entities.push_back(std::move(ent));

  kernel::Serializer serilizer{std::move(scene)};
  serilizer.Serialize("scene.datatype");

  MacOSMain();
}

void Application::MacOSMain() {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    NSMenu *bar = [NSMenu new];
    [NSApp setMainMenu:bar];
    NSMenuItem *appMenuItem = [NSMenuItem new];
    [bar addItem:appMenuItem];
    NSMenu *appMenu = [NSMenu new];
    [appMenuItem setSubmenu:appMenu];
    [appMenu addItemWithTitle:@"Quit"
                       action:@selector(terminate:)
                keyEquivalent:@"q"];

    NSRect frame = NSMakeRect(0, 0, _config.width, _config.height);
    NSWindowStyleMask styleMask =
        NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
        NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;

    NSWindow *window =
        [[NSWindow alloc] initWithContentRect:frame
                                    styleMask:styleMask
                                      backing:NSBackingStoreBuffered
                                        defer:NO];

    [window setTitle:[NSString stringWithUTF8String:_config.title.c_str()]];
    [window center];
    [window makeKeyAndOrderFront:nil];
    [window setAcceptsMouseMovedEvents:YES];
    [window setReleasedWhenClosed:NO];

    renderer::RendererConfig rConfig;
    rConfig.WindowHandle = (__bridge void *)window;
    rConfig.width = _config.width;
    rConfig.height = _config.height;

    _renderer->Initialize(rConfig);
    InitShaders();

    engineThread = std::thread(&Application::EngineThreadFunc, this);
    [NSApp finishLaunching];
    [NSApp activateIgnoringOtherApps:YES];

    double targetFrameTime = 1.0 / 120.0;
    while (_running) {
      @autoreleasepool {
        auto currentTime = std::chrono::high_resolution_clock::now();
        NSEvent *event;
        do {
          event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                     untilDate:[NSDate distantPast]
                                        inMode:NSDefaultRunLoopMode
                                       dequeue:YES];
          if (event) {
            if (event.type == NSEventTypeMouseMoved) {
              NSPoint loc = [window mouseLocationOutsideOfEventStream];
            }
            [NSApp sendEvent:event];
          }
        } while (event != nil);

        if (![window isVisible])
          _running = false;

        EngineUpdate();

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endTime - currentTime;
        if (elapsed.count() < targetFrameTime) {
          std::this_thread::sleep_for(
              std::chrono::duration<double>(targetFrameTime - elapsed.count()));
        }
      }
    }
  }
}

void Application::InitShaders() {
  auto Load = [](const std::string &path) -> std::vector<u32> {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
      return {};
    size_t size = (size_t)file.tellg();
    std::vector<u32> buffer(size / sizeof(u32));
    file.seekg(0);
    file.read((char *)buffer.data(), size);
    return buffer;
  };
  auto vWords = Load(std::string(SHADER_DIR) + "test.vert.spv");
  auto fWords = Load(std::string(SHADER_DIR) + "test.frag.spv");
  if (!vWords.empty() && !fWords.empty()) {
    _testVert = _renderer->CreateShader(vWords, renderer::ShaderStage::Vertex);
    _testFrag =
        _renderer->CreateShader(fWords, renderer::ShaderStage::Fragment);
  }
}

void Application::EngineUpdate() const {
  _renderer->BeginFrame();
  _renderer->Clear({0.1f, 0.1f, 0.1f, 1.0f});
  if (_testVert && _testFrag) {
    _renderer->BindShader(_testVert);
    _renderer->BindShader(_testFrag);
    _renderer->Draw();
    _renderer->DrawText("Testing this text thing", 100, 100, 1.0f, 1.0f, 1.0f,
                        1.0f);
  }
  _renderer->EndFrame();
}

void Application::EngineThreadFunc() {}
} // namespace arch::core::platform
