#include <stdio.h>
#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "os/os_core.h"

#include "base/base_string.cpp"
#include "base/base_arena.cpp"
#include "os/os_core.cpp"

#define AIM_PROFILER 1
#include "aim_profiler.h"
#include "aim_timer.h"
#include "aim_profiler.cpp"
#include "aim_timer.cpp"

#include <gl/gl.h>

#include<windows.h>

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x00000002
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define ERROR_INVALID_PROFILE_ARB               0x2096

typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int *attribList);
typedef const char *(WINAPI * PFNWGLGETEXTENSIONSSTRINGEXTPROC) (void);
typedef int (WINAPI * PFNWGLGETSWAPINTERVALEXTPROC) (void);

typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
typedef int (WINAPI * PFNWGLGETSWAPINTERVALEXTPROC) (void);


LRESULT WndProc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) 
{
    LRESULT result = {0};
    switch(message) {
        case WM_DESTROY: {
            PostQuitMessage(0);
        }break;
        default: {
            result = DefWindowProcW(window, message, w_param, l_param);
        }break;
    }
    return result;
}

void str8list_push(Arena* arena, Str8List* list, Str8 string) {
    Str8ListNode* node = 0;
    if(list->count == 0) 
    {
        //Str8ListNode* node = (Str8ListNode*) malloc(sizeof(Str8ListNode));
        //memset(node, 0, sizeof(Str8ListNode));

        Str8ListNode* node = (Str8ListNode*) arena_push_size(arena, Str8ListNode, 1);
        node->str = string;
        list->first = node;
        list->last = node;
        list->count++;
    }
    else
    {
        for (node = list->first; node != 0; node = node->next) {};
            //node = (Str8ListNode*) malloc(sizeof(Str8ListNode));
            //memset(node, 0, sizeof(Str8ListNode));
            node = (Str8ListNode*) arena_push_size(arena, Str8ListNode, 1);
            node->str = string;
            list->last->next = node;
            list->last = node;
            list->count++;
    }
}

void str8list_push_malloc(Str8List* list, Str8 string) {
    Str8ListNode* node = 0;
    if(list->count == 0) 
    {
        Str8ListNode* node = (Str8ListNode*) malloc(sizeof(Str8ListNode));
        memset(node, 0, sizeof(Str8ListNode));

        node->str = string;
        list->first = node;
        list->last = node;
        list->count++;
    }
    else
    {
        for (node = list->first; node != 0; node = node->next) {};
            node = (Str8ListNode*) malloc(sizeof(Str8ListNode));
            memset(node, 0, sizeof(Str8ListNode));
            node->str = string;
            list->last->next = node;
            list->last = node;
            list->count++;
    }
}

b32 is_running = true;
void Win32ProcessPendingMessages() {
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&Message);
        switch(Message.message) {
            case WM_QUIT: {
                is_running = false;
            }break;
            default: {
                DispatchMessageW(&Message);
            }break;
        }
    }
}

typedef void (WINAPI* PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
typedef void (WINAPI* PFNGLBINDVERTEXARRAYPROC) (GLuint array);

global_variable GLuint global_vao;
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, i32 show_code) {


    aim_profiler_begin();

    Arena arena = {};
    arena_init(&arena, mb(3));
    {
        WNDCLASSEXW wndclass {};
        wndclass.cbSize = sizeof(WNDCLASSEX);
        wndclass.style = CS_HREDRAW|CS_VREDRAW;
        wndclass.lpfnWndProc = WndProc;
        wndclass.hInstance = instance;
        wndclass.lpszClassName = L"graphical-window";
        RegisterClassExW(&wndclass);
    }
    HWND window = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"graphical-window",
        L"My window",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280,
        720,
        0, 0,
        instance,
        0
    );

    HDC hdc = GetDC(window);
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixel_format = ChoosePixelFormat(hdc, &pfd);
    if(pixel_format) {
        SetPixelFormat(hdc, pixel_format, &pfd);
    }else{
        __debugbreak();
    }


    HGLRC tempRC = wglCreateContext(hdc);
    if(wglMakeCurrent(hdc, tempRC))
    {
        // NOTE It seems that in order to get anything from `wglGetProcAddress`, `wglMakeCurrent` must have been called!
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");

        i32 attrib_list[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_FLAGS_ARB, 0,
            WGL_CONTEXT_PROFILE_MASK_ARB,
            WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0
        };
        HGLRC hglrc = wglCreateContextAttribsARB(hdc, 0, attrib_list);
        wglMakeCurrent(0, 0);
        wglDeleteContext(tempRC);
        wglMakeCurrent(hdc, hglrc);
    }
    
    PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) wglGetProcAddress("glGenVertexArrays"); 
    PFNGLBINDVERTEXARRAYPROC glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) wglGetProcAddress("glBindVertexArray"); 
    glGenVertexArrays(1, &global_vao);
    glBindVertexArray(global_vao);

    PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC) wglGetProcAddress("wglGetExtensionsStringEXT");

    const char* extensions = wglGetExtensionsStringEXT();
    Str8List* extensions_list = arena_push_size(&arena, Str8List, 1);

    b32 swap_control_supported = false;

    {
        aim_profiler_time_block("OpenGL Extensions extraction ARENA");
		const char* at = extensions;
        while(*at) 
        {
            while(*at == ' ') {
                at++;
            }

            const char* start = at;
            while(*at && *at != ' ') {
                at++; 
            }
            if (!*at) {
                break;
            }
            size_t count = at - start;
            Str8 extension = str8((char*)start, count);
            b32 res1 = str8_equals(extension, str8_lit("WGL_EXT_framebuffer_sRGB"));
            b32 res2 = str8_equals(extension, str8_lit("WGL_ARB_framebuffer_sRGB"));
            if(str8_equals(extension, str8_lit("WGL_EXT_swap_control")))
            {
                swap_control_supported = true;
            }

            str8list_push(&arena, extensions_list, extension);
        }
        i32 vsynch = -1;
        if(swap_control_supported) {
            PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
            PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC) wglGetProcAddress("wglGetSwapIntervalEXT");
            if(wglSwapIntervalEXT(1)) 
            {
                vsynch = wglGetSwapIntervalEXT();
                int x = 321;
            }
        }
    }

    aim_profiler_end();
    aim_profiler_print();

    while(is_running){
        Win32ProcessPendingMessages();
    }
    //PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC) wglGetProcAddress("wglGetSwapIntervalEXT");
}

