#include "Piecetable.hpp"

PieceTable PieceTable::create(Arena *arena, String8 original_file) {
  PieceTable pt = {};
  pt.table_arena = arena;
  pt.original = original_file;
  pt.add_buffer_arena = Arena::bootstrap(1024 * 1024 * 10); // 10MB Add Buffer
  
  pt.piece_capacity = 1000;
  pt.pieces = PushArray(arena, Piece, pt.piece_capacity);
  
  // Create first piece covering the entire original file
  pt.pieces[0] = {PieceSource::Original, 0, original_file.size};
  pt.piece_count = 1;
  
  return pt;
}

void PieceTable::insert(s64 offset, String8 text) {
  // 1. Copy text to Add Buffer
  void *dst = add_buffer_arena.push(text.size);
  u8* src = (u8*)text.str;
  for(s64 i = 0; i < text.size; ++i) ((u8*)dst)[i] = src[i];
  
  s64 add_start = add_buffer_size;
  add_buffer_size += text.size;

  // 2. Find which piece contains this offset
  s64 current_pos = 0;
  s64 piece_idx = -1;
  for (s64 i = 0; i < piece_count; ++i) {
    if (offset >= current_pos && offset <= current_pos + pieces[i].length) {
      piece_idx = i;
      break;
    }
    current_pos += pieces[i].length;
  }

  if (piece_idx != -1) {
    Piece target = pieces[piece_idx];
    s64 offset_in_piece = offset - current_pos;

    // Split target piece into two, and insert new piece in middle
    Piece p1 = {target.source, target.start, offset_in_piece};
    Piece p_new = {PieceSource::Add, add_start, text.size};
    Piece p2 = {target.source, target.start + offset_in_piece, target.length - offset_in_piece};

    // Shift array to make room for 2 more pieces (p_new and p2, since p1 replaces target)
    // In a production Piece Tree, this is a tree rotation
    for (s64 i = piece_count - 1; i > piece_idx; --i) {
      pieces[i + 2] = pieces[i];
    }

    pieces[piece_idx] = p1;
    pieces[piece_idx + 1] = p_new;
    pieces[piece_idx + 2] = p2;
    piece_count += 2;
  }
}

String8 PieceTable::get_all_text(Arena *scratch) {
  s64 total_size = 0;
  for(s64 i = 0; i < piece_count; ++i) total_size += pieces[i].length;

  char *buf = (char *)scratch->push(total_size + 1);
  s64 write_pos = 0;

  for (s64 i = 0; i < piece_count; ++i) {
    Piece p = pieces[i];
    const char *src = (p.source == PieceSource::Original) ? original.str : (const char *)add_buffer_arena.base;
    
    // Manual copy
    for (s64 j = 0; j < p.length; ++j) {
      buf[write_pos++] = src[p.start + j];
    }
  }
  buf[total_size] = 0;
  return String8(buf, total_size);
}

void PieceTable::remove(s64 offset, s64 length) {
    if (length <= 0) return;

    // Find pieces affected by the deletion
    s64 current_pos = 0;
    for (s64 i = 0; i < piece_count; ++i) {
        s64 piece_end = current_pos + pieces[i].length;

        if (offset >= current_pos && offset < piece_end) {
            s64 offset_in_piece = offset - current_pos;

            // Scenario: Deletion happens entirely inside one piece
            if (offset_in_piece + length < pieces[i].length) {
                Piece target = pieces[i];
                Piece p1 = {target.source, target.start, offset_in_piece};
                Piece p2 = {target.source, target.start + offset_in_piece + length, target.length - (offset_in_piece + length)};
                
                // Shift and insert p1 and p2 instead of target
                // [Array shift logic here...]
                pieces[i] = p1;
                // Move pieces up and insert p2 at i+1
                break;
            }
        }
        current_pos += pieces[i].length;
    }
}