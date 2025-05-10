#pragma once


struct Arena 
{
    u8* base;
    size_t len;
    size_t max_len;
    u8 temp_count;
};

struct TempArena 
{
    u64 pos;
    Arena* arena;
};


internal void arena_init(Arena* arena, size_t size, u8 *base = 0);
internal void* _push_size(Arena* arena, size_t size, u64 alignemnt = 8);
internal TempArena temp_begin(Arena* arena);
internal void temp_end(TempArena temp_arena);