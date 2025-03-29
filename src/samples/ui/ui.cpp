#include "stdio.h"
#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "os/os_core.h"
#include "draw/draw.h"
#include "font/font.h"

#include "base/base_string.cpp"
#include "base/base_arena.cpp"
#include "os/os_core.cpp"
#include "draw/draw.cpp"
#include "font/font.cpp"


global_variable OS_Window global_w32_window;
global_variable OS_PixelBuffer global_pixel_buffer;

///////////////// input //////////////////////

enum OS_Modifiers {
    OS_Modifiers_Ctrl = (1 << 0),
    OS_Modifiers_Alt = (1 << 1),
    OS_Modifiers_Shift = (1 << 2),
};

global_variable u32 os_modifiers;
enum Keys {
    Keys_A = 0x41,
    Keys_B = 0x42,
    Keys_C = 0x43,
    Keys_D = 0x44,
    Keys_E = 0x45,
    Keys_F = 0x46,
    Keys_G = 0x47,
    Keys_H = 0x48,
    Keys_I = 0x49,
    Keys_J = 0x4A,
    Keys_K = 0x4B,
    Keys_L = 0x4C,
    Keys_M = 0x4D,
    Keys_N = 0x4E,
    Keys_O = 0x4F,
    Keys_P = 0x50,
    Keys_Q = 0x51,
    Keys_R = 0x52,
    Keys_S = 0x53,
    Keys_T = 0x54,
    Keys_U = 0x55,
    Keys_V = 0x56,
    Keys_W = 0x57,
    Keys_X = 0x58,
    Keys_Y = 0x59,
    Keys_Z = 0x5A,
    Keys_Arrow_Left =	0x25,
    Keys_Arrow_Up = 0x26,
    Keys_Arrow_Right = 0x27,
    Keys_Arrow_Down = 0x28,
    // NOTE in practice it seems only Shift and Control are called and not the Left and Right variants for some reason!! Windows 11, keyboard: logitech g915 tkl lightsped
    Keys_Shift = 0x10,
    Keys_ShiftLeft = 0xA0,
    Keys_ShiftRight = 0xA1,
    Keys_Control = 0x11,
    Keys_ControlLeft = 0xA2,
    Keys_ControlRight = 0xA3,
    Keys_Alt = 0x12,
    Keys_Space = 0x20,
    Keys_Enter = 0x0D,
    Keys_Backspace = 0x08,
    Keys_Caps = 0x14,
    Keys_ESC = 0x1B,
    Keys_Count = 0xFE,
};


struct KeyboardState {
    u8 keys[Keys_Count];
};

enum Buttons {
    Buttons_LeftClick,
    Buttons_RightClick,
    Buttons_MiddleClick,
    Buttons_Count,
};

struct MouseState {
    u8 button[Buttons_Count];
    i32 x;
    i32 y;
    f32 scroll;
};

// TODO evaluate whether u32 or u8 is better for `button` and `keys`
struct Input {
    KeyboardState curr_keyboard_state;
    KeyboardState prev_keyboard_state;
    MouseState curr_mouse_state;
    MouseState prev_mouse_state;
};

void input_update(Input *input)
{
    input->prev_keyboard_state = input->curr_keyboard_state;
    input->prev_mouse_state = input->curr_mouse_state;
    memset(&input->curr_keyboard_state, 0, sizeof(input->curr_keyboard_state));
    memset(&input->curr_mouse_state, 0, sizeof(input->curr_mouse_state));
}

u32 input_click_left(Input* input) {
    u32 result = 0;
    result = input->curr_mouse_state.button[Buttons_LeftClick] == 1;
    return result;
}

u32 input_is_key_just_pressed(Input* input, Keys key) {
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 1 && input->prev_keyboard_state.keys[key] == 0;
    return result;
}

u32 input_is_key_just_released(Input* input, Keys key) {
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 0 && input->prev_keyboard_state.keys[key] == 1;
    return result;
}

u32 input_is_key_pressed(Input* input, Keys key) {
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 1;
    return result;
}

u32 input_is_key_released(Input* input, Keys key) {
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 0;
    return result;
}

u32 input_is_any_modifier_pressed(Input* input, u32 key) {
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

global_variable Input global_input;
///////////////// input //////////////////////

enum DebugVariableType
{
    DebugVariableType_B32,
    DebugVariableType_U32,
    DebugVariableType_F32,
    DebugVariableType_Group,
};

struct DebugVariable;
struct DebugGroup
{
    // this only make sense if its a group
    b32 expanded;
    DebugVariable *first_child;
    DebugVariable *last_child;
};

struct DebugVariable
{
    Str8 name;
    DebugVariableType type;
    DebugVariable *parent;
    DebugVariable *next;
    union
    {
        b32 b32_value;
        u32 u32_value;
        f32 f32_value;
        DebugGroup group;
    };
};

struct DebugState
{
    Arena arena;
    DebugVariable *root_var;
    DebugVariable *active;
    DebugVariable *hot;
    DebugVariable *next_hot;
};

struct DebugContext
{
    DebugState *state;
    DebugVariable *current_group;

};

internal DebugVariable * 
debug_add_var(DebugContext *context, Str8 name)
{
    DebugVariable *var;
    var = arena_push_size(&context->state->arena, DebugVariable, 1);
    return var;
}

internal DebugVariable * 
debug_add_var(DebugContext *context, Str8 name, DebugVariableType type)
{
    DebugVariable *var = debug_add_var(context, name);
    
    var->parent = context->current_group;
    var->name = name;
    var->type = type;

    DebugVariable *group = context->current_group;
    var->parent = group;
    if(group->group.last_child)
    {
        group->group.last_child->next = var;
        group->group.last_child = var;
    }
    else
    {
        group->group.last_child = group->group.first_child = var;
    }
    return var;
}

internal void
debug_begin_group(DebugContext *context, Str8 group_name)
{
    DebugVariable *var = debug_add_var(context, group_name, DebugVariableType_Group);
    var->group.expanded = true;
    context->current_group = var;
}


internal void 
debug_end_group(DebugContext *context)
{
    context->current_group = context->current_group->parent;
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

void debug_draw_menu(DebugVariable *var);
int main()
{
    Arena arena;
    arena_init(&arena, 1024 * 1024 * 2);

    u32 window_width = 1280;
    u32 window_height = 720;
    u32 screenWidth = GetSystemMetrics(SM_CXSCREEN);
    u32 screenHeight = GetSystemMetrics(SM_CYSCREEN);
    RECT window_rect = {0};
    SetRect(&window_rect,
    (screenWidth / 2) - (window_width / 2),
    (screenHeight / 2) - (window_height / 2),
    (screenWidth / 2) + (window_width / 2),
    (screenHeight / 2) + (window_height / 2));

    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, false);
    global_w32_window =  os_win32_open_window(window_rect, win32_main_callback);
    global_pixel_buffer = os_win32_create_buffer(window_width, window_height);

    // fonts

    font_init();
    FontInfo font_info = font_load(&arena);
    

    // debug info
    // How does he creates and assigns the DebugState's arena from the first one???
    DebugState *debug_state = arena_push_size(&arena, DebugState, 1);
    arena_init(&debug_state->arena, 1024 * 1024 * 1);
    DebugVariable *root_var = arena_push_size(&debug_state->arena, DebugVariable, 1);
    debug_state->root_var = root_var;


    DebugContext context; 
    context.state = debug_state;
    context.current_group = root_var;


    debug_begin_group(&context, str8("Group 1"));
    {
        debug_add_var(&context, str8("elem 1"), DebugVariableType_B32);
        debug_add_var(&context, str8("elem 2"), DebugVariableType_B32);
    }
    debug_end_group(&context);

    debug_begin_group(&context, str8("Group 2"));
    {
        debug_add_var(&context, str8("elem 1"), DebugVariableType_B32);
        debug_begin_group(&context, str8("Group 3"));
        {
            debug_add_var(&context, str8("elem 1"), DebugVariableType_B32);
        }
        debug_end_group(&context);
    }
    debug_end_group(&context);

    debug_add_var(&context, str8("groupless item"), DebugVariableType_B32);


    while (global_w32_window.is_running)
    {
        win32_process_pending_msgs();
        clear_buffer(&global_pixel_buffer, 0xFF000000);

        char* c  = "World!";
        draw_text(&global_pixel_buffer, 200, 300, c, font_info.font_table);

        char buf[100];
        if (input_is_key_just_pressed(&global_input, Keys_A))
        {
            sprintf(buf,  "Key pressed: A\n");
            OutputDebugStringA(buf);
        }
        if (input_is_key_pressed(&global_input, Keys_D))
        {
            sprintf(buf, "Key pressed: D\n");
            OutputDebugStringA(buf);
        }
        
        OS_Window_Dimension win_dim = os_win32_get_window_dimension(global_w32_window.handle);
        HDC device_context = GetDC(global_w32_window.handle);
        os_win32_display_buffer(device_context, &global_pixel_buffer, win_dim.width, win_dim.height);
        ReleaseDC(global_w32_window.handle, device_context);
        input_update(&global_input);
    };
    debug_draw_menu(debug_state->root_var);
}

void debug_draw_menu(DebugVariable *var)
{
    assert(var->group.first_child);
    var = var->group.first_child;
    u32 depth = 0;
    while(var)
    {
        for(u32 d = 0; d < depth; d++)
        {
            printf(" ");
            printf(" ");
            printf(" ");
            printf(" ");
        }
        printf("%.*s\n", (u32) var->name.size, var->name.str);
        if (var->type == DebugVariableType_Group && var->group.expanded)
        {
            if(var->group.first_child)
            {
                depth++;
                var = var->group.first_child;
            }
        }
        else 
        {
            while(true)
            {
                if(!var) break;
                if(var->next)
                {
                    var = var->next;
                    break;
                }else
                {
                    var = var->parent;
                    depth--;
                }
            }

        }

    }
}