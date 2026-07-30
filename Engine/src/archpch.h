#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// DirectX
#if ARCH_RENDERER_D3D12
#include <d3d12.h>
#include <dxgi_6.h>
#include "d3dx12.h"
#else
#include <d3d11.h>
#include <dxgi.h>
#endif
#include <d3dcompiler.h>
#include <wrl.h>

// C++ Standard
#include <iostream>
#include <memory>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <fstream>
#include <map>
#include <cstdio>
#include <cstdint>
#include <unordered_map>
#include <random>
#include <set>
#include <optional>

#ifdef NDEBUG
#define ARCH_ASSERT(condition) ((void)0)
#else
#include <stdio.h>
#include <stdlib.h>

#define ARCH_ASSERT(condition)                                                 \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "Assertion failed: %s\nFile: %s, Line: %d\n", #condition, \
              __FILE__, __LINE__);                                             \
      __debugbreak();                                                        \
    }                                                                          \
  } while (0)
#endif

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#define VK_NULL VK_NULL_HANDLE

using u64 = uint64_t;
using u32 = uint32_t;
using u8 = uint8_t;

#include "Math/ArchMath.h"
#include "Log.h"

#include "UUID.h"

// IMGUI
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#if ARCH_RENDERER_DX
#include "backends/imgui_impl_dx12.h"
#else
#include "backends/imgui_impl_dx11.h"
#endif
#include <zlib.h>
