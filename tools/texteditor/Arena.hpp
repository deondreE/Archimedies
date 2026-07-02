#pragma once
#include "Apptypes.hpp"
#include <format>

#ifndef ARENA_DEFAULT_ALIGNMENT
#define ARENA_DEFAULT_ALIGNMENT 8
#endif

struct Arena {
  u8 *base;
  u64 capacity;
  u64 pos;
  u64 commit_pos;

  static Arena bootstrap(u64 capacity);
  void release();

  void *push_align(u64 size, u64 align);
  void *push(u64 size);
  void pop_to(u64 size);
  void clear();

  u8 *current_ptr() { return base + pos; }
};

struct ArenaTemp {
  Arena *arena;
  u64 pos;

  ArenaTemp(Arena *a) : arena(a), pos(a->pos) {}
  ~ArenaTemp() { arena->pos = pos; }
};

#define PushStruct(arena, type) (type *)arena->push(sizeof(type))
#define PushArray(arena, type, count) (type *)arena->push(sizeof(type) * count)

///////////////
// String integration
String8 str8_pushfv(Arena *arena, const char *fmt, va_list args);
String8 str8_copy(Arena *arena, String8 src);
template <typename... Args>
String8 str8_pushf(Arena *arena, std::format_string<Args...> fmt,
                   Args &&...args) {
  // Format into a temporary std::string
  std::string s = std::format(fmt, std::forward<Args>(args)...);

  // Copy into Arena
  char *buf = (char *)arena->push(s.size() + 1);
  for (s64 i = 0; i < (s64)s.size(); ++i) {
    buf[i] = s[i];
  }
  buf[s.size()] = 0;

  return String8(buf, (s64)s.size());
}
