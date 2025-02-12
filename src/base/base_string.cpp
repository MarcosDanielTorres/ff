#include "base/base_string.h"

#define str8_lit(S) Str8{(u8*) S, sizeof(S) - 1}

Str8 str8(char* str) {
    Str8 result = {0};
    result.str = (u8*) str;
    result.size = cstring8_length(str);
    return result;
}

u64 cstring8_length(char* str) {
    u64 result = {0};
    while(*str++ != '\0') { result++; }
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