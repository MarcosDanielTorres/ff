#pragma once
#include <stdint.h>

#define global_variable static
#define internal static
#define local_persist static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;
typedef uint32_t b32;
typedef u32 b32;

#define kb(value) (value * 1024LL)
#define mb(value) (kb(value) * 1024LL)
#define gb(value) (mb(value) * 1024LL)

#define min(a, b) ((a) > (b) ? (b) : (a))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define array_count(a) (sizeof(a) / sizeof(*a))

#define align_pow2(x,b) (((x) + (b) - 1)&(~((b) - 1)))

#define assert(expr) do{if(!(expr)) {__debugbreak();}}while(0)
#define gui_assert(expr, msg) do{if(!(expr)) {MessageBox(0, msg, 0, MB_OK | MB_ICONERROR); __debugbreak();}}while(0)