#include "base/base_arena.h"

#define arena_push_size(arena, value, count, ...) (value*) _push_size(arena, sizeof(value) * count, ##__VA_ARGS__);
#define arena_push_copy(arena, size, source, ...) memcpy((void*) _push_size(arena, size, ##__VA_ARGS__), source, size);

void arena_init(Arena* arena, size_t size) 
{
    arena->base = (u8*)VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    arena->len = 0;
    arena->max_len = size;
    arena->temp_count = 0;
}

void* _push_size(Arena* arena, size_t size, u64 alignment)
{
    /*
        000000 
    */
    u64 result_ptr = (u64)arena->base + arena->len;
    u64 alignment_offset = 0;
    u64 alignment_mask = alignment - 1;
    if(result_ptr & alignment_mask)
    {
        // align
        alignment_offset = alignment - (result_ptr & alignment_mask);

    }
    size += alignment_offset;
    gui_assert(arena->len + size <= arena->max_len, "Arena size exceeded!");
    //memset(arena->base, 0, size);
    arena->len += size;
    void* result = (void*)(result_ptr + alignment_offset);
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