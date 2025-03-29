#pragma once

#include <windows.h>
#include <processthreadsapi.h>


struct OS_Window
{
    HWND handle;
    WINDOWPLACEMENT window_placement;
    b32 is_fullscreen = false;
    b32 is_running = true;
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


void os_win32_toggle_fullscreen(HWND handle);
OS_Window_Dimension os_win32_get_window_dimension(HWND handle);
OS_PixelBuffer os_win32_create_buffer(int width, int height);
void os_win32_display_buffer(HDC device_context, OS_PixelBuffer* buffer, i32 window_width, i32 window_height);
OS_Window os_win32_open_window(RECT rect);