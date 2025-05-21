#pragma once

// NOTE(Marcos): 
// This define is here so that Windows does not include its one min and max macro definitions which conflicts when using std::max and std::min
#define NOMINMAX
#include <windows.h>
#include <processthreadsapi.h>


typedef LRESULT (CALLBACK *WIN32MAINCALLBACK) (HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
typedef u32 WindowOpenFlags;
enum
{
    WindowOpenFlags_Centered = (1 << 0),
};

struct OS_Window
{
    HWND handle;
    WINDOWPLACEMENT window_placement;
    b32 is_fullscreen = false;
    b32 is_running = true;
    b32 focused = true;
};

struct OS_Window_Dimension
{
    i32 width;
    i32 height;
};


struct OS_PixelBuffer
{
    BITMAPINFO info;
    u8* pixels;
    i32 width;
    i32 height;
    i32 pitch;
};


static void os_win32_toggle_fullscreen(HWND handle);
static OS_Window_Dimension os_win32_get_window_dimension(HWND handle);
static OS_PixelBuffer os_win32_create_buffer(int width, int height);
static void os_win32_display_buffer(HDC device_context, OS_PixelBuffer* buffer, i32 window_width, i32 window_height);
static OS_Window os_win32_open_window(const char* window_name, u32 window_width, u32 window_height, WIN32MAINCALLBACK w32_main_callback, WindowOpenFlags flags, HINSTANCE instance = 0);