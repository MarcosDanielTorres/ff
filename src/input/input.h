#pragma once
enum OS_Modifiers {
    OS_Modifiers_Ctrl = (1 << 0),
    OS_Modifiers_Alt = (1 << 1),
    OS_Modifiers_Shift = (1 << 2),
};

enum Keys 
{
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

struct KeyboardState 
{
    u8 keys[Keys_Count];
};

enum MouseButtons
{
    MouseButtons_LeftClick,
    MouseButtons_RightClick,
    MouseButtons_MiddleClick,
    MouseButtons_Count,
};

struct MouseState 
{
    u8 button[MouseButtons_Count];
    f32 x;
    f32 y;
    f32 scroll;
};

// TODO evaluate whether u32 or u8 is better for `button` and `keys`
struct Input 
{
    KeyboardState curr_keyboard_state;
    KeyboardState prev_keyboard_state;
    MouseState curr_mouse_state;
    MouseState prev_mouse_state;
    f32 dx;
    f32 dy;
    // TODO Esto no va aca
    f32 dt;
};

// Just define a Input variable and call input_update and the end of the loop!
void input_update(Input *input)
{

    input->dx = 0.0f;
    input->dy = 0.0f;
    input->prev_keyboard_state = input->curr_keyboard_state;
    input->prev_mouse_state = input->curr_mouse_state;
    //memset(&input->curr_keyboard_state, 0, sizeof(input->curr_keyboard_state));
    // If i let this line uncommented i will only have a mouse position when im actively moving the mouse. Which is not what i want!
    // Input in general seems wrong
    //memset(&input->curr_mouse_state, 0, sizeof(input->curr_mouse_state));
    //memset(&input->curr_mouse_state.button, 0, sizeof(input->curr_mouse_state.button));
}

u32 input_click_left_down(Input* input) 
{
    u32 result = 0;
    result = input->curr_mouse_state.button[MouseButtons_LeftClick] == 1;
    return result;
}

u32 input_was_click_left_up(Input* input) 
{
    u32 result = 0;
    result = input->prev_mouse_state.button[MouseButtons_LeftClick] == 0;
    return result;
}

u32 input_was_click_left_down(Input* input) 
{
    u32 result = 0;
    result = input->prev_mouse_state.button[MouseButtons_LeftClick] == 1;
    return result;
}

u32 input_click_left_up(Input* input) 
{
    u32 result = 0;
    result = input->curr_mouse_state.button[MouseButtons_LeftClick] == 0;
    return result;
}

u32 input_is_click_left_just_pressed(Input* input) 
{
    u32 result = 0;
    result = input->curr_mouse_state.button[MouseButtons_LeftClick] == 1 && input->prev_mouse_state.button[MouseButtons_LeftClick] == 0;
    return result;
}

u32 input_is_key_just_pressed(Input* input, Keys key) 
{
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 1 && input->prev_keyboard_state.keys[key] == 0;
    return result;
}

u32 input_is_key_just_released(Input* input, Keys key) 
{
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 0 && input->prev_keyboard_state.keys[key] == 1;
    return result;
}

u32 input_is_key_pressed(Input* input, Keys key) 
{
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 1;
    return result;
}

u32 input_was_key_pressed(Input* input, Keys key) 
{
    u32 result = 0;
    result = input->prev_keyboard_state.keys[key] == 1;
    return result;
}

u32 input_is_key_released(Input* input, Keys key) 
{
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 0;
    return result;
}

u32 input_is_any_modifier_pressed(Input* input, u32 key) 
{
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