#pragma once

#include <windows.h>
#include <processthreadsapi.h>

global_variable HINSTANCE os_win32_instance;
global_variable WINDOWPLACEMENT global_window_pos = {sizeof(WINDOWPLACEMENT)};
global_variable b32 global_toggle_fullscreen = false;
global_variable HWND global_window_handle;

struct OS_Window 
{
    HWND handle;
};

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