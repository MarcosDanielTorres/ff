void os_win32_toggle_fullscreen(HWND handle, WINDOWPLACEMENT *window_placement) 
{
  DWORD window_style = GetWindowLong(handle, GWL_STYLE);
  if (window_style & WS_OVERLAPPEDWINDOW) 
  {
    MONITORINFO monitor_info = {sizeof(monitor_info)};
    if(GetWindowPlacement(handle, window_placement) && 
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
    SetWindowPlacement(handle, window_placement);
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

OS_PixelBuffer os_win32_create_buffer(int width, int height) {
    OS_PixelBuffer result = {0};
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

void os_win32_display_buffer(HDC device_context, OS_PixelBuffer* buffer, i32 window_width, i32 window_height) {
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



typedef LRESULT (CALLBACK *WIN32MAINCALLBACK) (HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

OS_Window os_win32_open_window(const char* window_name, u32 window_width, u32 window_height, WIN32MAINCALLBACK w32_main_callback, WindowOpenFlags flags) 
{
    RECT window_rect = {0};
    if(flags & WindowOpenFlags_Centered)
    {
        u32 screen_width = GetSystemMetrics(SM_CXSCREEN);
        u32 screen_height = GetSystemMetrics(SM_CYSCREEN);
        SetRect(&window_rect,
                (screen_width / 2) - (window_width / 2),
                (screen_height / 2) - (window_height / 2),
                (screen_width / 2) + (window_width / 2),
                (screen_height / 2) + (window_height / 2));
    } 

    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, false);

    OS_Window result = {0};
    WNDCLASSA WindowClass = {0};
    {
        WindowClass.style = CS_HREDRAW|CS_VREDRAW;
        WindowClass.lpfnWndProc = w32_main_callback;
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
        window_name, //[in, optional] LPCSTR    lpWindowName,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, //[in]           DWORD     dwStyle,
        //CW_USEDEFAULT, //[in]           int       X,
        //CW_USEDEFAULT, //[in]           int       Y,
        //rect.right - rect.left,//[in]           int       nWidth,
        //rect.bottom - rect.top, //[in]           int       nHeight,
        window_rect.left, //[in]           int       X,
        window_rect.top, //[in]           int       Y,
        window_rect.right - window_rect.left,//[in]           int       nWidth,
        window_rect.bottom - window_rect.top, //[in]           int       nHeight,
        0, //[in, optional] HWND      hWndParent,
        0, //[in, optional] HMENU     hMenu,
        // TODO Same here, is instance useful?
        0, //[in, optional] HINSTANCE hInstance,
        0 //[in, optional] LPVOID    lpParam
    );
    result.handle = handle;
    return result;
}