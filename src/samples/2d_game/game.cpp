
#include "stdio.h"
#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "os/os_core.h"
#include "draw/draw.h"
#include "font/font.h"
#include "aim_timer.h"

#include "base/base_string.cpp"
#include "base/base_arena.cpp"
#include "os/os_core.cpp"
#include "draw/draw.cpp"
#include "font/font.cpp"
#include "game_types.h"
#include "logic/game.h"
#include "aim_timer.cpp"


global_variable OS_Window global_w32_window;
global_variable OS_PixelBuffer global_pixel_buffer;
global_variable u32 os_modifiers;
global_variable GameInput global_input;

void input_update(GameInput *input)
{
    input->prev_keyboard_state = input->curr_keyboard_state;
    input->prev_mouse_state = input->curr_mouse_state;
    memset(&input->curr_keyboard_state, 0, sizeof(input->curr_keyboard_state));
    // If i let this line uncommented i will only have a mouse position when im actively moving the mouse. Which is not what i want!
    // GameInput in general seems wrong
    //memset(&input->curr_mouse_state, 0, sizeof(input->curr_mouse_state));
    //memset(&input->curr_mouse_state.button, 0, sizeof(input->curr_mouse_state.button));
}

u32 input_click_left_down(GameInput* input) {
    u32 result = 0;
    result = input->curr_mouse_state.button[Buttons_LeftClick] == 1;
    return result;
}

u32 input_click_left_up(GameInput* input) {
    u32 result = 0;
    result = input->curr_mouse_state.button[Buttons_LeftClick] == 0;
    return result;
}

u32 input_is_key_just_pressed(GameInput* input, Keys key) {
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 1 && input->prev_keyboard_state.keys[key] == 0;
    return result;
}

u32 input_is_key_just_released(GameInput* input, Keys key) {
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 0 && input->prev_keyboard_state.keys[key] == 1;
    return result;
}

u32 input_is_key_pressed(GameInput* input, Keys key) {
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 1;
    return result;
}

u32 input_is_key_released(GameInput* input, Keys key) {
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 0;
    return result;
}

u32 input_is_any_modifier_pressed(GameInput* input, u32 key) {
    u32 result = 0;
    result = (
        input_is_key_pressed(input, Keys_Shift) ||
        input_is_key_pressed(input, Keys_ShiftLeft) ||
        input_is_key_pressed(input, Keys_ShiftRight) ||
        input_is_key_pressed(input, Keys_ControlLeft) ||
        input_is_key_pressed(input, Keys_ControlRight)
    );
    return result;
}



void win32_process_pending_msgs() {
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&Message);
        switch(Message.message)
        {
            case WM_QUIT:
            {
                global_w32_window.is_running = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                WPARAM wparam = Message.wParam;
                LPARAM lparam = Message.lParam;

                u32 key = (u32)wparam;
                u32 alt_key_is_pressed = (lparam & (1 << 29)) != 0;
                u32 was_pressed = (lparam & (1 << 30)) != 0;
                u32 is_pressed = (lparam & (1 << 31)) == 0;
                global_input.curr_keyboard_state.keys[key] = u8(is_pressed);
                global_input.prev_keyboard_state.keys[key] = u8(was_pressed);
                if (GetKeyState(VK_SHIFT) & (1 << 15)) {
                    os_modifiers |= OS_Modifiers_Shift;
                }
                // NOTE reason behin this is that every key will be an event and each event knows what are the modifiers pressed when they key was processed
                // It the key is a modifier as well then the modifiers of this key wont containt itself. Meaning if i press Shift and i consult the modifiers pressed
                // for this key, shift will not be set
                if (GetKeyState(VK_CONTROL) & (1 << 15)) {
                    os_modifiers |= OS_Modifiers_Ctrl;
                }
                if (GetKeyState(VK_MENU) & (1 << 15)) {
                    os_modifiers |= OS_Modifiers_Alt;
                }
                if (key == Keys_Shift && os_modifiers & OS_Modifiers_Shift) {
                    os_modifiers &= ~OS_Modifiers_Shift;
                }
                if (key == Keys_Control && os_modifiers & OS_Modifiers_Ctrl) {
                    os_modifiers &= ~OS_Modifiers_Ctrl;
                }
                if (key == Keys_Alt && os_modifiers & OS_Modifiers_Alt) {
                    os_modifiers &= ~OS_Modifiers_Alt;
                }
            } break;

            case WM_MOUSEMOVE:
            {
                i32 xPos = (Message.lParam & 0x0000FFFF); 
                i32 yPos = ((Message.lParam & 0xFFFF0000) >> 16); 

                i32 xxPos = LOWORD(Message.lParam);
                i32 yyPos = HIWORD(Message.lParam);
                char buf[100];
                sprintf(buf,  "MOUSE MOVE: x: %d, y: %d\n", xPos, yPos);
                OutputDebugStringA(buf);

                assert((xxPos == xPos && yyPos == yPos));
                global_input.curr_mouse_state.x = xPos;
                global_input.curr_mouse_state.y = yPos;
            }
            break;
            case WM_LBUTTONUP:
            {
                global_input.curr_mouse_state.button[Buttons_LeftClick] = 0;

            } break;
            case WM_MBUTTONUP:
            {
                global_input.curr_mouse_state.button[Buttons_MiddleClick] = 0;
            } break;
            case WM_RBUTTONUP:
            {
                global_input.curr_mouse_state.button[Buttons_RightClick] = 0;
            } break;

            case WM_LBUTTONDOWN:
            {
                global_input.curr_mouse_state.button[Buttons_LeftClick] = 1;

            } break;
            case WM_MBUTTONDOWN:
            {
                global_input.curr_mouse_state.button[Buttons_MiddleClick] = 1;
            } break;
            case WM_RBUTTONDOWN:
            {
                global_input.curr_mouse_state.button[Buttons_RightClick] = 1;
            } break;
            default:
            {
                DispatchMessageA(&Message);
            } break;
        }
    }
}

LRESULT CALLBACK win32_main_callback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    switch(Message)
    {
        case WM_DESTROY:
        {
            global_w32_window.is_running = false;
        } break;
        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            OS_Window_Dimension Dimension = os_win32_get_window_dimension(Window);
            os_win32_display_buffer(DeviceContext, &global_pixel_buffer,
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
int main()
{
    // Arena arena;
    // arena_init(&arena, 1024 * 1024 * 2);

    u32 window_width = 1280;
    u32 window_height = 720;
    global_w32_window =  os_win32_open_window("Some basic game!", window_width, window_height, win32_main_callback, WindowOpenFlags_Centered);
    global_pixel_buffer = os_win32_create_buffer(window_width, window_height);

    // fonts
    // font_init();
    // FontInfo font_info = font_load(&arena);



    u64 permanent_mem_size = mb(10);
    u64 transient_mem_size = mb(20);
    u8 *memory = (u8*) VirtualAlloc(0, permanent_mem_size + transient_mem_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    u8 *permanent_mem = memory;
    u8 *transient_mem = (u8*)memory + permanent_mem_size;

    GameMemory game_memory = { 0 };
    game_memory.init = false;
    game_memory.permanent_mem_size = permanent_mem_size;
    game_memory.transient_mem_size = transient_mem_size;
    game_memory.permanent_mem = permanent_mem;
    game_memory.transient_mem = transient_mem;

    LONGLONG frequency = aim_timer_get_os_freq();
    LONGLONG time_now = aim_timer_get_os_time();

    while (global_w32_window.is_running)
    {
        time_now = aim_timer_get_os_time();

        win32_process_pending_msgs();

        main_game_loop(&global_pixel_buffer, &global_input, &game_memory);

        OS_Window_Dimension win_dim = os_win32_get_window_dimension(global_w32_window.handle);
        HDC device_context = GetDC(global_w32_window.handle);
        os_win32_display_buffer(device_context, &global_pixel_buffer, win_dim.width, win_dim.height);
        ReleaseDC(global_w32_window.handle, device_context);
        input_update(&global_input);
        LONGLONG dt_long = aim_timer_get_os_time() - time_now;
        global_input.dt = aim_timer_ticks_to_sec(dt_long, frequency);
        //printf("%f.2fms\n", aim_timer_ticks_to_ms(dt_long, frequency));
    }
}