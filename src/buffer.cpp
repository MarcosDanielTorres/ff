#include "buffer.h"

internal void gapbuffer_init(GapBuffer* buffer, Arena* arena, size_t size)
{
    size_t cap = align_pow2(20 * 2, kb(4));
    buffer->mem = arena_push_size(arena, u8, cap);
    buffer->gap_start = 10;
    buffer->gap_end = 20;
    buffer->gap_size = 20 - 10;
    buffer->capacity = cap;
}