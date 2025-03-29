#pragma once
struct GapBuffer
{
    u8* mem;
    size_t gap_start;
    size_t gap_end;
    size_t gap_size;
    size_t capacity;
};


internal void gapbuffer_init(GapBuffer* buffer, Arena* arena, size_t size);