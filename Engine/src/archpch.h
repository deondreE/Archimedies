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
#include <memory>
#include <cmath>
#include <sstream>
#include <fstream>
#include <zlib.h>
#include <map>
#include <cstdio>
#include <cstdint>
#include <unordered_map>
#include <random>

using u64 = uint64_t;
using u32 = uint32_t;
using u8 = uint8_t;

#include "Math/ArchMath.h"
#include "Log.h"

// IMGUI
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#if ARCH_RENDERER_DX
#include "backends/imgui_impl_dx12.h"
#else
#include "backends/imgui_impl_dx11.h"
#endif