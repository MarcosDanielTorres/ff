#include "base/base_arena.h"

#define arena_push_size(arena, value, count) (value*) _push_size(arena, sizeof(value) * count);

void arena_init(Arena* arena, u8* memory, size_t max_len) 
{
    arena->base = memory;
    arena->len = 0;
    arena->max_len = max_len;
    arena->temp_count = 0;
}

void* _push_size(Arena* arena, size_t size) 
{
    assert(arena->len + size <= arena->max_len);
    void* result = arena->base + arena->len;
    memset(arena->base, 0, size);
    arena->len += size;
    return result;
}


TempArena temp_begin(Arena* arena) 
{
    TempArena result = {0};
    result.arena = arena;
    result.pos = arena->len;
    arena->temp_count++;
    return result;
}

void temp_end(TempArena temp_arena) 
{
    assert(temp_arena.arena->temp_count > 0);
    //memset(temp_arena.arena->base, 0, temp_arena.arena->len);
    temp_arena.arena->len = temp_arena.pos;
    temp_arena.arena->temp_count--;
}