#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// DirectX
#include <d3d11.h>
#include <dxgi.h>
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
#include <map>
#include <cstdio>
#include <unordered_map>

#include "Math/ArchMath.h"
#include "Log.h"

// IMGUI
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
