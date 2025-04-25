#include "base/base_string.h"

#define str8_lit(S) Str8{(u8*) S, sizeof(S) - 1}

Str8 str8(const char* str, size_t size) {
    Str8 result = {0};
    result.str = (u8*) str;
    result.size = size;
    return result;
}

/*
TODO
    ver que psaria si hago

    Str8 some; 
    {
        char *str = "hola"; 
        some = str8(str);
    }
    printf("%s\n", some.str);

*/
Str8 str8(const char* str) {
    Str8 result = {0};
    result.str = (u8*) str;
    result.size = cstring8_length(str);
    return result;
}

Str8 str8_fmt(Arena *arena, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Str8 result = {0};
    u32 thankyouchatgpt = _vscprintf(fmt, args);
    result.size = thankyouchatgpt;
    result.str = (u8*)arena_push_size(arena, u8, result.size);
    int comomevasadecirqueson17bytes = vsprintf((char*)result.str, fmt, args);
    //char buf[300];
    //result.str = (u8*)buf;
    va_end(args);
    return result;
}

u64 cstring8_length(const char* str) {
    u64 result = {0};
    const char *iter = str;
    while(*iter++ != '\0') { result++; }
    return result;
}

u64 cstring16_length(wchar_t* str) {
    u64 result = {0};
    while(*str++ != '\0') { result++; }
    return result;
}

Str16 str16_lit(wchar_t* str_literal) {
    Str16 result = {0};
    result.size = cstring16_length(str_literal);
    result.str = (u16*) str_literal;
    return result;
}

b32 str8_equals(Str8 str_a, Str8 str_b) {
    b32 result = false;
    if (str_a.size == str_b.size) {
        result = true;
        for(i32 i = 0; i < str_a.size; i++) {
            if (str_a.str[i] != str_b.str[i]) {
                result = false;
                break;
            }
        }
    }

    return result;
}


// TODO fix this
Str8 str8_fmt(char* fmt, char* str)
{
    Str8 result = {0};
    char buf[100];
    char *end = buf + sizeof(buf);
    result.size = _snprintf_s(buf, (size_t)(end - buf), (size_t)(end - buf), fmt, str);
    return result;
}
