#include "Application.h"
#include "IShader.h"
#include <fstream>
#include <print>
#include <string>
#include <utility>
#include <vector>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#elif defined(PLATFORM_LINUX)
#include <X11/xlib.h>
#elif defined(PLATFORM_MACOS)
#else
#error "Unsupported platform!"
#endif

// @Todo: Wayland support for Linux.

namespace arch::core::platform {
Application::Application(ApplicationConfig config)
    : _config(std::move(config)) {
  _renderer = renderer::IRenderer::Create();
  _meshLoader = std::make_unique<kernel::MeshLoader>();
  _meshLoader->LoadMesh(std::string(MODEL_DIR) + "test.glb");
}

Application::~Application() = default;

void Application::EngineUpdate() const {
  _renderer->BeginFrame();

  // _renderer->Clear(renderer::Color::FromRGB(33, 33, 33)); // Set clear color

  if (_testVert && _testFrag) {
    _renderer->BindShader(_testVert);
    _renderer->BindShader(_testFrag);
    _renderer->Draw(); // <--- THIS must be called
  }
  // _renderer->Clear(renderer::Color::FromRGB(255, 0, 0, 255));
  _renderer->EndFrame();
}

void Application::InitShaders() {
  auto LoadSPIRV = [](const std::string &path) -> std::vector<u32> {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      std::print("Failed to open shader: {}\n", path);
      return {};
    }

    size_t fileSize = static_cast<size_t>(file.tellg());

    if (fileSize % sizeof(u32) != 0) {
      std::print("Warning: Shader size for {} is not a multiple of 4 bytes.\n",
                 path);
    }

    std::vector<u32> buffer(fileSize / sizeof(u32));

    file.seekg(0);
    file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    file.close();

    return buffer;
  };

  const std::string vPath = std::string(SHADER_DIR) + "test.vert.spv";
  const std::string fPath = std::string(SHADER_DIR) + "test.frag.spv";

  std::print("Loading Vertex from: {}\n", vPath);

  std::vector<u32> vertWords = LoadSPIRV(vPath);
  if (vertWords.empty()) {
    std::print("FAILED to load Vertex Shader from path: {}\n", vPath);
    return;
  }

  std::vector<u32> fragWords = LoadSPIRV(fPath);
  if (fragWords.empty()) {
    std::print("FAILED to load Fragment Shader!\n");
    return;
  }

  _testVert = _renderer->CreateShader(vertWords, renderer::ShaderStage::Vertex);
  _testFrag =
      _renderer->CreateShader(fragWords, renderer::ShaderStage::Fragment);
}

#ifdef PLATFORM_WINDOWS
void Application::WindowsMain() {
  std::print("[ARCHIMEDES] Running on Windows!");

  MSG msg{0};
  while (_running) {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      if (msg.message == WM_QUIT) {
        _running = false;
      }
    }

    EngineUpdate();
  }
}
#elif defined(PLATFORM_LINUX)
void Application::LinuxMain() {
  std::print("Running on Linux!");

  Display *display = XOpenDisplay(nullptr);
  if (!display) {
    std::print("Failed to open X display!");
    return;
  }

  while (_running) {
    while (XPending(display)) {
      XEvent event;
      XNextEvent(display, &event);

      if (event.type == ClientMessage) {
        _running = false;
      }
    }

    EngineUpdate();
  }

  XCloseDisplay(display);
}
#elif defined(PLATFORM_MACOS)
void MacOSMain();
#else
#error "Unsupported platform!"
#endif

// I need the to understand what platform you are on so that the platform main
// can be called here.
void Application::Run() {
#ifdef PLATFORM_WINDOWS
  WindowsMain();
#elif defined(PLATFORM_LINUX)
  LinuxMain();
#elif defined(PLATFORM_MACOS)
  MacOSMain();
#else
#error "Unsupported platform!"
#endif
}
} // namespace arch::core::platform
