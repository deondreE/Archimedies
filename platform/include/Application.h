#pragma once
#include "IRenderer.h"
#include "Mesh.h"
#include <atomic>
#include <memory>
#include <string>

namespace arch::core::platform {
struct ApplicationConfig {
  std::string title = "Archimedes Engine";
  int width = 800;
  int height = 600;
};

class Application {
public:
  Application(ApplicationConfig config = {});
  ~Application();

  void Run();
  void Stop() { _running = false; }
  void EngineUpdate() const;
  void InitShaders();

private:
#ifdef PLATFORM_WINDOWS
  void WindowsMain();
#elif defined(PLATFORM_MACOS)
  void MacOSMain();
#elif defined(PLATFORM_LINUX)
  void LinuxMain();
#endif
  void EngineThreadFunc();
  std::thread engineThread;
  std::atomic<bool> _running = true;

  std::unique_ptr<renderer::IRenderer> _renderer;
  std::unique_ptr<kernel::MeshLoader> _meshLoader;

  ApplicationConfig _config;
  std::shared_ptr<renderer::IShader> _testVert;
  std::shared_ptr<renderer::IShader> _testFrag;
};
} // namespace arch::core::platform