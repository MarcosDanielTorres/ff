#include "base/base_os.h"


void os_win32_toggle_fullscreen(HWND handle) 
{
  DWORD window_style = GetWindowLong(handle, GWL_STYLE);
  if (window_style & WS_OVERLAPPEDWINDOW) 
  {
    MONITORINFO monitor_info = {sizeof(monitor_info)};
    if(GetWindowPlacement(handle, &global_window_pos) && 
        GetMonitorInfo(MonitorFromWindow(handle, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
    {
      SetWindowLong(handle, GWL_STYLE, window_style & ~WS_OVERLAPPEDWINDOW);
      SetWindowPos(handle, HWND_TOP,
                   monitor_info.rcMonitor.left,
                   monitor_info.rcMonitor.top,
                   monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                   monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
  }
  else
  {
    SetWindowLong(handle, GWL_STYLE, window_style | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(handle, &global_window_pos);
    SetWindowPos(handle, 0, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
}

OS_Window_Dimension os_win32_get_window_dimension(HWND handle) {
    OS_Window_Dimension result = {0};
    RECT rect = {0};
    GetClientRect(handle, &rect);
    result.width = rect.right - rect.left;
    result.height = rect.bottom - rect.top;
    return result;
}

OS_Window_Buffer os_win32_create_buffer(int width, int height) {
    OS_Window_Buffer result = {0};
    BITMAPINFO info = {0};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    u8* pixels = (u8*)VirtualAlloc(0 , width * height * 4, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

    result.info = info;
    result.pixels = pixels;
    result.width = width;
    result.height = height;
    result.pitch = width * 4;
    return result;
}

void os_win32_display_buffer(HDC device_context, OS_Window_Buffer* buffer, i32 window_width, i32 window_height) {
    if(window_width >= buffer->width * 2 && 
       window_height >= buffer->height * 2)
    {
        StretchDIBits(
            device_context,
            0, 0, buffer->width * 2, buffer->height * 2,
            0, 0, buffer->width, buffer->height,
            buffer->pixels,
            &buffer->info,
            DIB_RGB_COLORS, SRCCOPY
        );
    } else {
        StretchDIBits(
            device_context,
            0, 0, buffer->width, buffer->height,
            0, 0, buffer->width, buffer->height,
            buffer->pixels,
            &buffer->info,
            DIB_RGB_COLORS, SRCCOPY
        );
    }

}


global_variable OS_Window_Buffer global_buffer;


OS_FileReadResult os_file_read(Arena* arena, const char* filename) 
{
    OS_FileReadResult result = {0};

    FILE* file = fopen(filename, "rb");
    if (file)  
    {
        fseek(file, 0, SEEK_END);
        size_t filesize = ftell(file);
        fseek(file, 0,SEEK_SET);

        //u8* buffer = (u8*)malloc(filesize + 1);
        u8* buffer = (u8*) arena_push_size(arena, u8, filesize + 1);
        fread(buffer, 1, filesize, file);

        buffer[filesize] = '\0';

        result.data = buffer;
        result.size = filesize;
        fclose(file);
    }
    return result;
}
