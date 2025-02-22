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

global_variable OS_Window global_os_w32_window;

struct OS_Window_Dimension 
{
    i32 width;
    i32 height;
};

struct OS_Window_Buffer 
{
    BITMAPINFO info;
    u8* pixels;
    i32 width;
    i32 height;
    i32 pitch;
};

struct OS_FileReadResult 
{
    u8* data;
    size_t size;
};

void os_win32_toggle_fullscreen(HWND handle);
OS_Window_Dimension os_win32_get_window_dimension(HWND handle);
OS_Window_Buffer os_win32_create_buffer(int width, int height);
void os_win32_display_buffer(HDC device_context, OS_Window_Buffer* buffer, i32 window_width, i32 window_height);
OS_FileReadResult os_file_read(Arena* arena, const char* filename); 
OS_Window os_win32_open_window(RECT rect);