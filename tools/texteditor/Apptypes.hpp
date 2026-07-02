#pragma once

#include <SDL.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "String8.hpp"

//// Basic types

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define UNUSED(x) (void)(x)
#define TEXT_EDITOR_VERSION "0.1.0"

struct Color {
  u8 r, g, b, a;
};

struct Vec2 {
  float x, y;
};

struct Vec2i {
  int x, y;
};

#define StaticAssert(expr, msg) static_assert(expr, msg)
#define ASSERT(expr, msg) static_assert(expr, msg)

#ifdef __APPLE__
#define PATH_SEPARATOR "/"
#define DEFAULT_FONT_PATH                                                      \
  "/Users/deondreenglish/Library/Fonts/MapleMono-Regular.ttf"
#else
#define PATH_SEPARATOR "\\"
#define DEFAULT_FONT_PATH "C:\\Windows\\Fonts\\MapleMono-Regular.ttf"
#endif
