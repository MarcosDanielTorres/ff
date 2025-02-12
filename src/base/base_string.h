#pragma once

struct Str8 
{
    u8* str;
    size_t size;
};

struct Str8ListNode {
    Str8ListNode* next;
    Str8 str;
};

struct Str8List {
    Str8ListNode* first;
    Str8ListNode* last;
    u64 count;
};

struct Str16 
{
    u16* str;
    size_t size;
};


u64 cstring8_length(char* str);
b32 str8_equals(Str8 str_a, Str8 str_b);