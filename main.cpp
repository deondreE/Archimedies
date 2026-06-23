#include "Application.h"

using namespace arch::core::platform;

int main() {
  Application *app = new Application();
  app->Run();

  delete app;

  return 0;
}
