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

    //TODO 
    result.str = (u8*)arena_push_size(arena, u8, result.size);
    // Lo puedo reemplazar por: ????
    //result.str = (u8*)arena_push_copy(arena, result.size, (u8*) buf);?????

    vsprintf((char*)result.str, fmt, args);
    //char buf[300];
    //result.str = (u8*)buf;
    va_end(args);
    return result;
}


// todo completar
void str8_to_cstring(Str8 a, u8* buf)
{
    for(u32 i = 0; i < a.size; i++)
    {
        *buf++ = a.str[i]; 
    }
    *buf = '\0';
}

Str8 str8_concat(Arena *arena, Str8 a, Str8 b)
{
    Str8 result = {0};
    result.size = a.size + b.size;
    // no arena alternative 
    //char *buf = (char*)malloc(200);
    // arena alternative 
    char buf[200];

    /*
        TODO
        otra alternativa es hacer lo que se hace en concat:
        pushea un size el tamanio de a + b y despues lo lleno de cosas y retorno la arena... 

        aca en estos ejemplos creo que el push copy no va, ni en concat ni en este porque 
        el copy es copiarle algo a la memoria despues de hacer un push size, el tema
        es que no tengo nada que copiar porque esta vacio

        Supongo que no se usa copy en estos dos casos, ver! Donde mieraa uso un push_copy entonces?
        Por ahora solo lo use en vulkan, se me ocurren ejemplos pero no es este el caso!
    */
    char *at = buf;
    for(u32 i = 0; i < a.size; i++)
    {
        *at++ = a.str[i];
    }
    for(u32 i = 0; i < b.size; i++)
    {
        *at++ = b.str[i];
    }
    // no arena alternative 
    // result.data = (u8*)buf;
    // arena alternative 
    result.str = (u8*)arena_push_copy(arena, result.size, (u8*)buf);
    return result;
}
const char * 
str8_to_cstring(Arena *arena, Str8 str)
{
    char *result = (char*) arena_push_copy(arena, str.size + 1, str.str);
    result[str.size] = '\0';
    return (const char*) result;
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
