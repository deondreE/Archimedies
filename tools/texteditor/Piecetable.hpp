#pragma once
#include "Arena.hpp"

enum class PieceSource : u8 { Original, Add };

struct Piece {
  PieceSource source;
  s64 start;
  s64 length;
};

struct PieceTable {
  String8 original;
  Arena add_buffer_arena;
  s64 add_buffer_size;

  Piece *pieces;
  s64 piece_count;
  s64 piece_capacity;
  Arena *table_arena;

  static PieceTable create(Arena *arena, String8 original_file);
  void insert(s64 offset, String8 text);
  void remove(s64 offset, s64 length);

  String8 get_all_text(Arena *scratch);
};