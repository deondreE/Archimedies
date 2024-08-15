#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw.h>

// #include <string>

namespace arc {
  class Window  {
    public:
      Window(int w, int h, std::string name);
      ~Window();
    
    private: 
      void initWindow();

      const int width; 
      const int height;

      std::string windowName;
      GLFWwindow *window;
  }; 
};