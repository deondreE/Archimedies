#include "Arena.hpp"
#include <stdarg.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

inline u64 align_forward(u64 ptr, u64 align) {
  return (ptr + (align - 1)) & ~(align - 1);
}

Arena Arena::bootstrap(u64 capacity) {
  Arena arena = {};
  arena.capacity = capacity;

#ifdef _WIN32
  arena.base = (u8 *)VirtualAlloc(NULL, capcity, MEM_RESERVE, PAGE_READWRITE);
#else
  arena.base = (u8 *)mmap(NULL, capacity, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
  arena.pos = 0;
  arena.commit_pos = 0;
  return arena;
}

void Arena::release() {
#ifdef _WIN32
  VirtualFree(base, 0, MEM_RELEASE);
#else
  munmap(base, capacity);
#endif
}

void *Arena::push_align(u64 size, u64 align) {
  u64 current = pos;
  u64 aligned_pos = align_forward(current, align);
  u64 next_pos = aligned_pos + size;

  if (next_pos <= capacity) {
    // Commit physical memory pages if needed
    if (next_pos > commit_pos) {
      u64 commit_increment = align_forward(next_pos - commit_pos, 4096);
#ifdef _WIN32
      VirtualAlloc(base + commit_pos, commit_increment, MEM_COMMIT,
                   PAGE_READWRITE);
#else
      mprotect(base + commit_pos, commit_increment, PROT_READ | PROT_WRITE);
#endif
      commit_pos += commit_increment;
    }

    void *result = base + aligned_pos;
    pos = next_pos;
    return result;
  }

  // ERROR: In a real editor, you'd trigger a crash/assert here
  return nullptr;
}

void *Arena::push(u64 size) {
  return push_align(size, ARENA_DEFAULT_ALIGNMENT);
}

void Arena::pop_to(u64 p) {
  if (p < pos)
    pos = p;
}

void Arena::clear() { pop_to(0); }

String8 str8_copy(Arena *arena, String8 src) {
  if (src.size == 0)
    return String8();

  char *buffer = (char *)arena->push(src.size + 1);

  for (s64 i = 0; i < src.size; ++i) {
    buffer[i] = src.str[i];
  }

  buffer[src.size] = 0; // Null terminate

  return String8(buffer, src.size);
}
