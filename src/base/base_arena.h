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


void arena_init(Arena* arena, size_t size);
void* _push_size(Arena* arena, size_t size);
TempArena temp_begin(Arena* arena);
void temp_end(TempArena temp_arena);