#include "base/base_os.h"


void os_win32_toggle_fullscreen(HWND handle) 
{
  DWORD window_style = GetWindowLong(handle, GWL_STYLE);
  if (window_style & WS_OVERLAPPEDWINDOW) 
  {
    MONITORINFO monitor_info = {sizeof(monitor_info)};
    if(GetWindowPlacement(handle, &global_os_w32_window.window_placement) && 
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
    SetWindowPlacement(handle, &global_os_w32_window.window_placement);
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

LRESULT CALLBACK os_win32_main_callback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    switch(Message)
    {
        case WM_DESTROY:
        {
            global_os_w32_window.is_running = false;
        } break;
        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            OS_Window_Dimension Dimension = os_win32_get_window_dimension(Window);
            os_win32_display_buffer(DeviceContext, &global_buffer,
                                       Dimension.width, Dimension.height);
            EndPaint(Window, &Paint);
        }break;
        default:
        {
            result = DefWindowProcA(Window, Message, wParam, lParam);
        } break;
    }
    return result;
}

OS_Window os_win32_open_window(RECT rect) {
    OS_Window result = {0};
    WNDCLASSA WindowClass = {0};
    {
        WindowClass.style = CS_HREDRAW|CS_VREDRAW;
        WindowClass.lpfnWndProc = os_win32_main_callback;
        // TODO I don't know if this is useful or not
        //WindowClass.hInstance = instance;
        WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
        WindowClass.lpszClassName = "graphical-window";
        RegisterClass(&WindowClass);
    }

    DWORD style = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    HWND handle = CreateWindowExA(
        0,
        "graphical-window", //[in, optional] LPCSTR    lpClassName,
        "jaja!!", //[in, optional] LPCSTR    lpWindowName,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, //[in]           DWORD     dwStyle,
        //CW_USEDEFAULT, //[in]           int       X,
        //CW_USEDEFAULT, //[in]           int       Y,
        //rect.right - rect.left,//[in]           int       nWidth,
        //rect.bottom - rect.top, //[in]           int       nHeight,
        rect.left, //[in]           int       X,
        rect.top, //[in]           int       Y,
        rect.right - rect.left,//[in]           int       nWidth,
        rect.bottom - rect.top, //[in]           int       nHeight,
        0, //[in, optional] HWND      hWndParent,
        0, //[in, optional] HMENU     hMenu,
        // TODO Same here, is instance useful?
        0, //[in, optional] HINSTANCE hInstance,
        0 //[in, optional] LPVOID    lpParam
    );
    result.handle = handle;
    global_os_w32_window.handle = handle;
    return result;
}