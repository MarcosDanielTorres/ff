#include <stdio.h>
#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "base/base_os.h"

#include "base/base_string.cpp"
#include "base/base_arena.cpp"
#include "base/base_os.cpp"


union FlagsUnion {
    struct {
        u32 pressed_left   : 1;
        u32 pressed_middle : 1;
        u32 pressed_right  : 1;
    };
    u32 all_flags;
};


Str8 os_g_key_display_string_table[143] =
{
    str8_lit("Invalid Key"),
    str8_lit("Escape"),
    str8_lit("F1"),
    str8_lit("F2"),
    str8_lit("F3"),
    str8_lit("F4"),
};



global_variable FT_Library library;
global_variable Arena global_arena;
global_variable int is_running = true;

global_variable char* global_line;
global_variable u32 global_cursor_in_line;
global_variable u32 global_curr_line_number = 0;
global_variable u32 global_max_line_length = 300;


struct Cursor {
    u32 line_number;
    u32 rel_line_pos;
};

struct Line {
    char* line;
    u32 cursor_pos;
};

struct TextEditor {
    Line* lines;
    u32 line_count {1};
    Cursor cursor;
};

// NOTE this is a premature optmization i could have used itoa or sprintf but its for educational purpose so fuck off!!!
struct LineNumberToChars {
    char* buf;
    u32 key_line_number;
    LineNumberToChars* next;
};

u32 line_numbers_to_chars_hash(int num) {
    u32 result = {0};
    return result;
}

void add_line_number_in_chars(LineNumberToChars* line_numbers_to_chars, u32 index, u32 key_line_number, char* buf) {
    LineNumberToChars* slot;
    for(slot = &line_numbers_to_chars[index];; slot = slot->next) {
        if(slot && slot->buf == 0) {
            slot->buf = buf;
            slot->key_line_number = key_line_number;
            break;
        }

        if(slot == 0) {
            slot = (LineNumberToChars*) malloc(sizeof(LineNumberToChars));
            slot->buf = buf;
            slot->key_line_number = key_line_number;
            break;
        }
    }
}

LineNumberToChars* get_line_number_in_chars(LineNumberToChars* line_numbers_to_chars, u32 index, u32 key_line_number) {
    LineNumberToChars* result = 0;
    LineNumberToChars* slot = line_numbers_to_chars + index;
    for(;slot;slot = slot->next) {
        if (slot->buf && slot->key_line_number == key_line_number) {
            result = slot;
        }
    }
    return result;
}

LineNumberToChars line_numbers_to_chars[256];


enum OS_Modifiers {
    OS_Modifiers_Ctrl = (1 << 0),
    OS_Modifiers_Alt = (1 << 1),
    OS_Modifiers_Shift = (1 << 2),
};

global_variable u32 os_modifiers;

global_variable TextEditor text_editor;
///////////////// input //////////////////////
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


u32 is_key_just_pressed(Input* input, Keys key) {
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 1 && input->prev_keyboard_state.keys[key] == 0;
    return result;
}

u32 is_key_just_released(Input* input, Keys key) {
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 0 && input->prev_keyboard_state.keys[key] == 1;
    return result;
}

u32 is_key_pressed(Input* input, Keys key) {
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 1;
    return result;
}

u32 is_key_released(Input* input, Keys key) {
    u32 result = 0;
    result = input->curr_keyboard_state.keys[key] == 0;
    return result;
}

u32 is_any_modifier_pressed(Input* input, u32 key) {
    u32 result = 0;
    result = is_key_pressed(input, Keys_Shift) || is_key_pressed(input, Keys_ShiftLeft) || is_key_pressed(input, Keys_ShiftRight) || is_key_pressed(input, Keys_ControlLeft) || is_key_pressed(input, Keys_ControlRight);
    return result;
}
///////////////// input //////////////////////

global_variable Input global_input;


struct FontBitmap {
    i32 width;
    i32 height;
    i32 pitch;
    u8* buffer;
};

struct LoadedGlyph {
    FontBitmap bitmap;
    i32 bitmap_top;
    i32 advance_x;
};

global_variable LoadedGlyph letter_A_glyph;
global_variable LoadedGlyph letter_B_glyph;
global_variable LoadedGlyph letter_C_glyph;
global_variable LoadedGlyph letter_D_glyph;
global_variable LoadedGlyph letter_E_glyph;
global_variable LoadedGlyph letter_F_glyph;
global_variable LoadedGlyph letter_j_glyph;
global_variable LoadedGlyph letter_h_glyph;
global_variable LoadedGlyph letter_u_glyph;
global_variable LoadedGlyph letter_g_glyph;
global_variable LoadedGlyph letter_e_glyph;

global_variable LoadedGlyph font_table[300];


struct SomeRes 
{
    u8* bytes;
};

u8* some(Arena* arena) {
    TempArena temp_arena = temp_begin(arena);
    u8* bytes = arena_push_size(arena, u8, 10); // no entiendo por que cuando pushea no incrementa el pos del temp
    for(int i = 0; i < 10; i++) {
        u8* value = bytes + i;
        *value = 'a' + i;
    }

    for(int i = 0; i < 10; i++) {
        printf("%c, ", bytes[i]);
    }
    printf("\n");
    temp_end(temp_arena);
    return bytes;
}


global_variable char* test_string = (char*)malloc(100);
global_variable u32 test_string_count;

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    switch(Message)
    {
        case WM_DESTROY:
        {
            is_running = false;
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

OS_Window os_win32_open_window(i32 width, i32 height, RECT rect) {
    OS_Window result = {0};
    WNDCLASSA WindowClass = {0};
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    // TODO I don't know if this is useful or not
    //WindowClass.hInstance = instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";
    RegisterClass(&WindowClass);
    HWND handle = CreateWindowExA(
        0,
        WindowClass.lpszClassName, //[in, optional] LPCSTR    lpClassName,
        "Lucho!!", //[in, optional] LPCSTR    lpWindowName,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, //[in]           DWORD     dwStyle,
        CW_USEDEFAULT, //[in]           int       X,
        CW_USEDEFAULT, //[in]           int       Y,
        // TODO take this out from here
        rect.right - rect.left,//[in]           int       nWidth,
        rect.bottom - rect.top, //[in]           int       nHeight,
        0, //[in, optional] HWND      hWndParent,
        0, //[in, optional] HMENU     hMenu,
        // TODO Same here, is instance useful?
        0, //[in, optional] HINSTANCE hInstance,
        0 //[in, optional] LPVOID    lpParam
    );
    result.handle = handle;
    global_window_handle = handle;
    return result;
}

char* keys_to_str[6] =  {
    "Shift",
    "LeftShift",
    "RightShift",
    "Ctrl",
    "LeftCtrl",
    "RightCtrl"
};

char* keycode_to_str(Keys key) {
    char* result = 0;
    switch(key) 
    {
        case Keys_Shift: 
        {
            result = keys_to_str[0];
        } break;
        case Keys_ShiftLeft: 
        {
            result = keys_to_str[1];
        } break;
        case Keys_ShiftRight: 
        {
            result = keys_to_str[2];
        } break;
        case Keys_Control: 
        {
            result = keys_to_str[3];
        } break;
        case Keys_ControlLeft: 
        {
            result = keys_to_str[4];
        } break;
        case Keys_ControlRight: 
        {
            result = keys_to_str[5];
        } break;
    }
    return result;
}

Line* get_current_line(TextEditor* text_editor) {
    Line* result = 0;
    result = &text_editor->lines[text_editor->cursor.line_number];
    return result;
}

void add_char_to_line(TextEditor* text_editor, Line* line, char ch){
    line->line[text_editor->cursor.rel_line_pos++]  = ch;
}

u32 remove_char_from_line(Line* line, u32 rel_line_pos, u32 count = 1, char replace_by = char(32)) {
    u32 chars_removed = {0};
    u32 initial_pos = rel_line_pos;
    for(i32 i = 0; i < count; i++) {
        line->line[--initial_pos]  = replace_by;
    }
    return chars_removed = count;
}

void reset_current_cursor(TextEditor* text_editor) {
    text_editor->cursor.line_number++;
    text_editor->line_count++;
    text_editor->cursor.rel_line_pos = 0;
}


void Win32ProcessPendingMessages() {
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&Message);
        switch(Message.message)
        {
            case WM_QUIT:
            {
                is_running = false;
            } break;

            case WM_CHAR: 
            {
                /*
                Posted to the window with the keyboard focus when a WM_KEYDOWN message is translated by the TranslateMessage function. The WM_CHAR message contains the character code of the key that was pressed.

                wParam
                The character code of the key.

                lParam
                The repeat count, scan code, extended-key flag, context code, previous key-state flag, and transition-state flag, as shown in the following table.

                Bits	Meaning
                0-15	The repeat count for the current message. The value is the number of times the keystroke is autorepeated as a result of the user holding down the key. If the keystroke is held long enough, multiple messages are sent. However, the repeat count is not cumulative.
                16-23	The scan code. The value depends on the OEM.
                24	Indicates whether the key is an extended key, such as the right-hand ALT and CTRL keys that appear on an enhanced 101- or 102-key keyboard. The value is 1 if it is an extended key; otherwise, it is 0.
                25-28	Reserved; do not use.
                29	The context code. The value is 1 if the ALT key is held down while the key is pressed; otherwise, the value is 0.
                30	The previous key state. The value is 1 if the key is down before the message is sent, or it is 0 if the key is up.
                31	The transition state. The value is 1 if the key is being released, or it is 0 if the key is being pressed.
                */

                // NOTE lets assume its not needed for the application to know previous and current state as per the function definition the WM_CHAR is sent when WM_KEYDOWN
                // is translated by the TranslateMessage function!
                WPARAM wparam = Message.wParam;
                LPARAM lparam = Message.lParam;
                char ch = char(wparam);

                // NOTE When I press enter in Windows the character that gets written is '\r' so I will just convert it to '\n'. Although
                // when I'll load this I may have some trouble!!!
                // Questions: If I do this shouldn't I have to revert this when saving the file? Also why I'm converting it in the first place?
                // Guess: I believe its because of standarization and I'll convert it back when I save based on the OS. Not quite sure about this, should think it through!!
                if(ch == '\r') 
                {
                    ch = '\n';
                }

                // NOTE raddbg do not proccess '\n' and '\t'. And I guess the reason for that is because they don't allow text editing of files (as far as I can tell!)
                if((ch >= ' ' && ch != 127) || ch == '\n' || ch == '\t') 
                {
                    u32 bitmask_29 = (1 << 29);
                    if (global_cursor_in_line < global_max_line_length) 
                    {
                        Line* line = get_current_line(&text_editor);
                        add_char_to_line(&text_editor, line, ch);
                    }
                }

                if(is_key_pressed(&global_input, Keys_Backspace)) {
                    Line* line = get_current_line(&text_editor);
                    u32 chars_removed = remove_char_from_line(line, text_editor.cursor.rel_line_pos);
                    text_editor.cursor.rel_line_pos -= chars_removed;
                }

                if(is_key_pressed(&global_input, Keys_Enter)) {
                    reset_current_cursor(&text_editor);
                }
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

                char* key_to_str = keycode_to_str(Keys(key));
                if(key_to_str)
                {
                    char buf[300];
                    //sprintf(buf, "Modifier key processed! %s\n", key_to_str);
                    u32 shift_is_pressed = (os_modifiers & OS_Modifiers_Shift) != 0;
                    if(shift_is_pressed) {
                        sprintf(buf, "Modifier key pressed: Shift! \n");
                        OutputDebugStringA(buf);
                    }
                }


                /*
                // TODO dont include modifieres only alphabet and space
                u32 not_a_modifier_pressed = is_any_modifier_pressed(&input, key) == 0;
                if (not_a_modifier_pressed) {
                    // TODO this should inherit the parent cursor position. For now im just setting it to 0
                    if (is_key_pressed(&input, Keys_Enter)){
                        text_editor.cursor.line_number++;
                        text_editor.cursor.rel_line_pos = 0;
                        text_editor.line_count++;
                    }
                    else if (is_key_pressed(&input, Keys_Backspace)) {
                        Line* line = &text_editor.lines[text_editor.cursor.line_number];
                        line->line[--text_editor.cursor.rel_line_pos] = char(Keys_Space);
                        // global_line[--global_cursor_in_line] = char(Keys_Space);
                    }else{
                        if (global_cursor_in_line < global_max_line_length && is_pressed) {
                            Line* line = &text_editor.lines[text_editor.cursor.line_number];
                            line->line[text_editor.cursor.rel_line_pos++]  = key;
                            //global_line[global_cursor_in_line++] = char(key);
                        }
                    }
                }
                */

            } break;

            case WM_MOUSEMOVE:
            {
                i32 xPos = (Message.lParam & 0x0000FFFF); 
                i32 yPos = ((Message.lParam & 0xFFFF0000) >> 16); 

                i32 xxPos = LOWORD(Message.lParam);
                i32 yyPos = HIWORD(Message.lParam);
                char buf[100];
                sprintf(buf,  "MOUSE MOVE: x: %d, y: %d\n", xPos, yPos);
                //OutputDebugStringA(buf);


                assert((xxPos == xPos && yyPos == yPos));
            }
            break;
            

            default:
            {
                DispatchMessageA(&Message);
            } break;
        }
    }
}



struct GameState 
{
    int x;
    int y;
};

void draw_rect(OS_Window_Buffer* buffer, i32 dest_x, i32 dest_y, i32 width, i32 height, u32 color) {
    u8* row = buffer->pixels + buffer->pitch * dest_y + dest_x * 4;

    for(i32 y = 0; y < height; y++) {
        u32* pixel = (u32*)row;
        for(i32 x = 0; x < width; x++) {
            *pixel++ = color;
        }
        row += buffer->pitch;
    }
}

enum UI_Button_Flags {
    UI_Button_Flags_Drag = (1 << 0),
    UI_Button_Flags_Hover = (1 << 1),
};

struct UI_Button {
    // TODO should this be u32 or UI_Button_Flags
    u32 ui_flags;
    i32 x;
    i32 y;
    i32 w;
    i32 h;
    u32 color;
    const char* text;
};


/*
    Some thoughts:
    I could find where im at and only do this check once before drawing the button as in practice you wont be hovering more than one button at a time!

    But.. that would create another codepath, a codepath where draw_button takes a hovered colored vs non hovered codepath


    Another thing is to keep using only one draw_button with the flag check buuuuuuut the mouse state is inside the button this time!
    For this before drawing i need to do a pass inside every button and if mouse_inside_rect() break... the problem is that all the other buttons must be resseted!
    so i still need to go over them!
*/
/*
inline bool32
IsInRectangle(rectangle2 Rectangle, v2 Test)
{
    bool32 Result = ((Test.x >= Rectangle.Min.x) &&
                     (Test.y >= Rectangle.Min.y) &&
                     (Test.x < Rectangle.Max.x) &&
                     (Test.y < Rectangle.Max.y));

    return(Result);
}
*/



b32 mouse_inside_rect(i32 x, i32 y, i32 w, i32 h) {
    b32 result = ((global_input.curr_mouse_state.x > x) &&
                  (global_input.curr_mouse_state.x < w) && 
                  (global_input.curr_mouse_state.y > y) &&
                  (global_input.curr_mouse_state.y < h));
    return result;
}

void draw_button(OS_Window_Buffer* buffer, UI_Button button) {
    int dest_x = button.x;
    int dest_y = button.y;
    int height = button.h;
    int width = button.w;
    int color = button.color;
    int hovered_color = 0xFF00FFFF;

    if(button.ui_flags & UI_Button_Flags_Hover) {
        if (mouse_inside_rect(dest_x, dest_y, width, height)) {
            color = hovered_color;
        }
    }

    u8* row = buffer->pixels + buffer->pitch * dest_y + dest_x * 4;
    for(i32 y = 0; y < height; y++) {
        u32* pixel = (u32*)row;
        for(i32 x = 0; x < width; x++) {
            *pixel++ = color;
        }
        row += buffer->pitch;
    }
}


void draw_line(OS_Window_Buffer* buffer, i32 x1, i32 y1, i32 x2, i32 y2) {
    // NOTE: only supports rectas
    u8* row =  buffer->pixels + y1 * buffer->pitch + x1 * 4;
    u32* pixel = (u32*) row;
    while(x1 < x2) {
        *pixel++ = 0xFFFFFFFF;
        x1++;
    }
}


// Copies the `FontBitmap` into the `OS_Window_Buffer`
/*
* - face->glyph->bitmap
* --- face->glyph->bitmap.buffer
* - Global Metrics: face->(ascender, descender, height): For general font information
* - Scaled Global Metrics: face->size.metrics.(ascender, descender, height): For rendering text
* - "The metrics found in face->glyph->metrics are normally expressed in 26.6 pixel format"
    to convert to pixels >> 6 (divide by 64). Applies to scaled metrics as well.
* - Calling `FT_Set_Char_Size` sets `FT_Size` in the active `FT_Face` and is used by functions like: `FT_Load_Glyph`
    https://freetype.org/freetype2/docs/reference/ft2-sizing_and_scaling.html#ft_size
* - face->bitmap_top is the distance from the top of the bitmap to the baseline. y+ positive
    https://freetype.org/freetype2/docs/reference/ft2-glyph_retrieval.html#ft_glyphslot
* - glyph->advance.x must be shifted >> 6 as well if pixels are wanted
*/

void draw_bitmap(OS_Window_Buffer* buffer, i32 x_baseline, i32 y_baseline, LoadedGlyph hdp) {
    // TODO there is no transparency in the fonts!!!!!
    i32 new_y = y_baseline - hdp.bitmap_top;

    u8* dest_row = buffer->pixels + new_y * buffer->pitch + x_baseline * 4;
    for(i32 y = 0; y < hdp.bitmap.height; y++){
        u32* dest_pixel = (u32*) dest_row;
        for(i32 x = 0; x < hdp.bitmap.width; x++) {
            u8* src_pixel = hdp.bitmap.buffer + hdp.bitmap.width * y + x;
            u32 alpha = *src_pixel << 24;
            u32 red = *src_pixel << 16;
            u32 green = *src_pixel << 8;
            u32 blue = *src_pixel;
            u32 color = alpha | red | green | blue;
            *dest_pixel++ = color;
        }
        dest_row += buffer->pitch;
    }
}



LoadedGlyph load_glyph(FT_Face face, char codepoint) {
    LoadedGlyph result = {0};
    // The index has nothing to do with the codepoint
    u32 glyph_index = FT_Get_Char_Index(face, codepoint);
    if (glyph_index) 
    {
        FT_Error load_glyph_err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT );
        if (load_glyph_err) 
        {
            const char* err_str = FT_Error_String(load_glyph_err);
            printf("FT_Load_Glyph: %s\n", err_str);
        }

        FT_Error render_glyph_err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (render_glyph_err) 
        {
            const char* err_str = FT_Error_String(render_glyph_err);
            printf("FT_Load_Glyph: %s\n", err_str);
        }

        result.bitmap.width = face->glyph->bitmap.width;
        result.bitmap.height = face->glyph->bitmap.rows;
        result.bitmap.pitch = face->glyph->bitmap.pitch;
        
        result.bitmap_top = face->glyph->bitmap_top;
        result.advance_x = face->glyph->advance.x;

        FT_Bitmap* bitmap = &face->glyph->bitmap;
        result.bitmap.buffer = (u8*)malloc(result.bitmap.pitch * result.bitmap.height);
        memcpy(result.bitmap.buffer, bitmap->buffer, result.bitmap.pitch * result.bitmap.height);


    }
    return result;
}

size_t cstring_length(char* str) {
    size_t result = 0;
    for(char* c = str; *c != '\0'; c++) {
        result++;
    }
    return result;
}


struct CmdLine {

};

void EntryPoint();

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode) {
    i32 argc = __argc;
    char** argv = __argv;

    b32 paso_por_hola = false;
    for(i32 i = 0; i < argc; i++) {
        Str8 command = str8_lit(argv[i]);
        if (paso_por_hola) {
            if(str8_equals(command, str8_lit("hola"))) {
                int x =12312;
            }
        }

        if (str8_equals(command, str8_lit("--help"))) {
            paso_por_hola = true;
        }
    }

    os_win32_instance = Instance;
    LPSTR command_line = GetCommandLine();
    // cycle through the commands and everytime a space is found do createa String8 and place it in an array of String* commands;
    // 
    // My doubt is how to cleanly get the arguments for each command, because show could have a command or two or three and also -s is show as well. How to handle this
    // is not enough to just separate every command into its own string
    // unless you know that show is gonna possible have 3 args so you say: Cmd show_cmd
    // Cmd* args and to know which type of args you are working with you then get the type with a tagged union... which of course is messy as fuck!!

    // ["--help", "--show", "-s"]

    // 
    EntryPoint();
    FlagsUnion u;
    u.all_flags = 0;

    printf("Before setting anything: 0x%X\n", u.all_flags);
    u.pressed_left = 1;
    printf("After setting pressed_left: 0x%X\n", u.all_flags);

    u.all_flags |= (1 << 1);  // Set pressed_middle (if it's at bit position 1)
    printf("After setting pressed_middle: 0x%X\n", u.all_flags);


    memset(test_string, 0, 100);
    size_t arena_total_size = 1024 * 1024 * 1024;
    u8* memory =(u8*) malloc(arena_total_size);

    arena_init(&global_arena, memory, arena_total_size);

    FT_Error error = FT_Init_FreeType(&library);
    if (error)
    {
        const char* err_str = FT_Error_String(error);
        printf("FT_Init_FreeType: %s\n", err_str);
        exit(1);
    }

{

    GameState game = {0};
    game.x = 4;
    game.y = 3;

    uint64_t ptr_to_game = (uint64_t) &game;
    GameState* same_game = (GameState*) ptr_to_game;

    /*  What exactly the difference between a uint64_t pointer and a uint64_t value? Both are 8 bytes.
	    Because initially I thought I had to cast &game to uint64_t*, not to a uint64_t.
    */
}

{
    enum Type 
    {
        Type1,
        Type2,
    }; 
    struct Params 
    {
        Type type;
        union {
            struct 
            {
                int x,w;
            } type1;
            struct Type2
            {
                int x,y,z;
            }type2;
        };
    };

    Params t1_params = {0};
    t1_params.type = Type1;
    t1_params.type1.x = 11;
    t1_params.type1.w = 12;

    Params t2_params = {0};
    t2_params.type = Type2;
    t2_params.type2.x = 100;
    t2_params.type2.y = 101;
    t2_params.type2.z = 102;

    t2_params.type1.w = 102;

    struct Type1Params 
    {
        int x;
        int w;
    };

    struct Type2Params 
    {
        int x;
        int y;
        int z;
    };

    typedef struct ParamsM 
    {
        enum Type type;
        union {
            struct Type1Params t1;
            struct Type2Params t2;
        }; //data;
    } ParamsM;

    ParamsM t1_paramsM = {0};
    t1_paramsM.type = Type1;
    t1_paramsM.t1.x = 11;
    t1_paramsM.t1.w = 12;

    ParamsM t2_paramsM = {0};
    t2_paramsM.type = Type2;
    t2_paramsM.t2.x = 100;
    t2_paramsM.t2.y = 101;
    t2_paramsM.t2.z = 102;

    t2_paramsM.t1.w = 102;

    int x = 321321;
}



    int window_width = 1280;
    int window_height = 720;
    RECT window_rect = {0};
    window_rect.right = window_width;
    window_rect.bottom = window_height;
    AdjustWindowRect(&window_rect, 0, false);
    OS_Window window = os_win32_open_window(window_width, window_height, window_rect);

    // Allen stored the target width and height in a variable here calling win32_resize. But I guess its not needed
    // because at least as far as i can tell it `AdjustWindowRect` respects the fucking size!
    
    global_buffer = os_win32_create_buffer(window_width, window_height);


    {
        // NOTE For now I will keep using this logic. But it would be better if I can receive a string of 8 bits instead of a 16 bit
        // for that i need a way of translating u8 <-> u16 which is not trivial!!
        // references:
        // https://github.com/EpicGamesExt/raddebugger/blob/a1e7ec5a0e9c8674f5b0271ce528f6b651d43564/src/os/core/win32/os_core_win32.c#L256
        // https://github.com/EpicGamesExt/raddebugger/blob/a1e7ec5a0e9c8674f5b0271ce528f6b651d43564/src/base/base_strings.c#L1547
        // https://github.com/EpicGamesExt/raddebugger/blob/a1e7ec5a0e9c8674f5b0271ce528f6b651d43564/src/base/base_strings.c#L161
        // https://github.com/EpicGamesExt/raddebugger/blob/a1e7ec5a0e9c8674f5b0271ce528f6b651d43564/src/base/base_strings.h#L24
        // https://github.com/EpicGamesExt/raddebugger/blob/a1e7ec5a0e9c8674f5b0271ce528f6b651d43564/src/base/base_strings.h#L127
        // Also see: utf8_encode, utf8_decode and utf16 variants

        Str8 thread_name = str8_lit("aim thread 8 bits");
        Str16 thread_name16 = str16_lit(L"aim thread 16 bits");
        size_t wc_len = wcslen(L"aim thread 16 bits");
        assert(wc_len == thread_name16.size);

        HRESULT r;
        r = SetThreadDescription(
            GetCurrentThread(),
            (wchar_t*)thread_name16.str
        );
        r = SetThreadDescription(
            GetCurrentThread(),
            (wchar_t*)thread_name.str
        );
        r = SetThreadDescription(
            GetCurrentThread(),
            (wchar_t*)thread_name16.str
        );

        // TODO esta mierda ni anda
        {
                ULONG_PTR info[4];
                info[0] = 0x1000;               // dwType
                info[1] = (ULONG_PTR)"hola";
                info[2] = (ULONG_PTR)-1; 
                info[3] = 0;              
            #pragma warning(push)
            #pragma warning(disable: 6320 6322)
                __try
                {
                RaiseException(0x406D1388, 0, sizeof(info) / sizeof(void *), (const ULONG_PTR *)&info);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                }
            #pragma warning(pop)
        }

    }


    {
        OS_FileReadResult font_file = os_file_read(&global_arena, "C:\\Windows\\Fonts\\Arial.ttf");

        FT_Face face = {0};
        if(font_file.data)
        {
            FT_Open_Args args = {0};
            args.flags = FT_OPEN_MEMORY;
            args.memory_base = (u8*) font_file.data;
            args.memory_size = font_file.size;

            FT_Error opened_face = FT_Open_Face(library, &args, 0, &face);
            if (opened_face) 
            {
                const char* err_str = FT_Error_String(opened_face);
                printf("FT_Open_Face: %s\n", err_str);
                exit(1);
            }


            FT_Error set_char_size_err = FT_Set_Char_Size(face, 4 * 64, 4 * 64, 300, 300);

            u32 char_code = 0;
            u32 min_index = UINT32_MAX;
            u32 max_index = 0;
            for(;;) 
            {
                u32 glyph_index = 0;
                char_code = FT_Get_Next_Char(face, char_code, &glyph_index);
                if (char_code == 0) 
                {
                    break;
                }
                // here each char_code corresponds to ascii representantion. Also the glyph index is the same as if I did:
                // `FT_Get_Char_Index(face, 'A');`. So A would be: (65, 36)
                //printf("(%d, %d)\n", char_code, glyph_index);
                min_index = min(min_index, glyph_index);
                max_index = max(max_index, glyph_index);
            }
            printf("min and max indexes: (%d, %d)\n", min_index, max_index);

            for(u32 glyph_index = min_index; glyph_index <= max_index; glyph_index++) 
            {
                FT_Error load_glyph_err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT );
                if (load_glyph_err) 
                {
                    const char* err_str = FT_Error_String(load_glyph_err);
                    printf("FT_Load_Glyph: %s\n", err_str);
                }

                FT_Error render_glyph_err = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL);
                if (render_glyph_err) 
                {
                    const char* err_str = FT_Error_String(render_glyph_err);
                    printf("FT_Load_Glyph: %s\n", err_str);
                }
            }

            {
                /* TODOs
                * - Draw the baseline as a line
                * - Create render bitmap    antes habia hecho un render bitmap como viene del formato .bmp pero ahora no es necesario porque ya tengo un array de pixels.
                esto a pesar de compartir el nombre no es un bitmap de windows. Solo son pixels
                * - Draw a letter
                */

                /* NOTEs
                * - face->glyph->bitmap
                * --- face->glyph->bitmap.buffer
                * - Global Metrics: face->(ascender, descender, height): For general font information
                * - Scaled Global Metrics: face->size.metrics.(ascender, descender, height): For rendering text
                * - "The metrics found in face->glyph->metrics are normally expressed in 26.6 pixel format"
                    to convert to pixels >> 6 (divide by 64). Applies to scaled metrics as well.
                * - Calling `FT_Set_Char_Size` sets `FT_Size` in the active `FT_Face` and is used by functions like: `FT_Load_Glyph`
                    https://freetype.org/freetype2/docs/reference/ft2-sizing_and_scaling.html#ft_size
                * - face->bitmap_top is the distance from the top of the bitmap to the baseline. y+ positive
                    https://freetype.org/freetype2/docs/reference/ft2-glyph_retrieval.html#ft_glyphslot
                * - glyph->advance.x must be shifted >> 6 as well if pixels are wanted
                */

                // Inspect this word: "HogellTo"
                const char* word = "HogellTo";
                const char* c = word;
                //for(; *c != '\0'; c++) 
                //{
                    //u32 glyph_index = FT_Get_Char_Index(face, 'A');
                    //if (glyph_index) 
                    //{
                    //    FT_Error load_glyph_err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT );
                    //    if (load_glyph_err) 
                    //    {
                    //        const char* err_str = FT_Error_String(load_glyph_err);
                    //        printf("FT_Load_Glyph: %s\n", err_str);
                    //    }

                    //    FT_Error render_glyph_err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
                    //    if (render_glyph_err) 
                    //    {
                    //        const char* err_str = FT_Error_String(render_glyph_err);
                    //        printf("FT_Load_Glyph: %s\n", err_str);
                    //    }

						//letter_A_glyph//.glyph = face->glyph;
						//letter_A_glyph//.bitmap.width = face->glyph->bitmap.width;
						//letter_A_glyph//.bitmap.height = face->glyph->bitmap.rows;
						//letter_A_glyph//.bitmap.pitch = face->glyph->bitmap.pitch;
						//letter_A_glyph//.bitmap.buffer = face->glyph->bitmap.buffer;
                    //}
                //}
            }

            for(u32 codepoint = '!'; codepoint <= '~'; codepoint++) {
                font_table[u32(codepoint)] = load_glyph(face, char(codepoint));
            }
            font_table[u32(' ')] = load_glyph(face, char(' '));

        }
        
        global_line = (char*) malloc(global_max_line_length);
        memset(global_line, 0, global_max_line_length);

        text_editor.lines = (Line*) malloc(255 * sizeof(Line));
        for(u32 line_index = 0; line_index < 255; line_index++) {
            Line* line = text_editor.lines + line_index;
            line->line = (char*) malloc(global_max_line_length);
            memset(line->line, 0, global_max_line_length);
        }
        //Line* lines;
        //char* line;
        //u32 cursor_pos;
        //u32 line_count;
        //Cursor cursor;
        while(is_running) 
        {

            POINT mouse_point;
            DWORD LastError;
            if (GetCursorPos(&mouse_point) && ScreenToClient(global_window_handle, &mouse_point))
            {
                int fjdksal = 1231;
            }
            if (GetCursorPos(&mouse_point) && ScreenToClient(global_window_handle, &mouse_point))
            {

                char buf2[100];
                sprintf(buf2,  "converted x: %d, y: %d\n", mouse_point.x, mouse_point.y);
                OutputDebugStringA(buf2);
                int fjdksal = 1231;
            }else{
                LastError = GetLastError();
            }
            Win32ProcessPendingMessages();

            // NOTE No olvidar de llamar esto aca no ser pelotudo por dios!!!!!!
            draw_rect(&global_buffer, 0, 0, global_buffer.width, global_buffer.height, 0xFF000000);
            {

                // editor
                i32 y_baseline = 25;

                char* word = "Sale Dota eeh!";
                word = "The sound of ocean waves calms my soul";
                word = "The sound of the ocean";
                word = "GameUpdateAndRender?? hola";
                //for(char* c = global_line; *c != '\0'; c++) {
                u32 last_line_index = text_editor.line_count;

                // line_number = 120;
                // char number = "120";
                for(u32 line_index = 0; line_index < last_line_index; line_index++) {
                    i32 column_numbers_size = 30;
                    // TODO this `init_pos` should go away when I have the parent sorted.

                    ///////////////////////////// line numbers //////////////////////////////////
                    /* 
                    NOTE line numbers in vim: it has a size for the column numbers that grows based on the total line_count of the file

                    */
                    // NOTE instead of a hash I could have used: char* number_to_str[1000000000] = 
                    u32 index = line_numbers_to_chars_hash(line_index + 1) % array_count(line_numbers_to_chars);
                    LineNumberToChars* line_in_chars = get_line_number_in_chars(line_numbers_to_chars, index, line_index + 1);
                    char* buf = (char*)malloc(10);
                    if(line_in_chars) {
                        buf = line_in_chars->buf;
                    } else {
                        itoa(line_index + 1, buf, 10);
                        add_line_number_in_chars(line_numbers_to_chars, index, line_index + 1, buf);
                    }
                    size_t buf_size = cstring_length(buf);
                    u32 char_offset = 0;
                    u32 pos = 0;
                    for(i32 i = buf_size - 1; i >= 0; i--, pos++) {
                        LoadedGlyph glyph = font_table[(u32)buf[i]];
                        char_offset += glyph.advance_x >> 6;
                        draw_bitmap(&global_buffer, column_numbers_size - char_offset, y_baseline * (line_index + 1), glyph);
                    }
                    ///////////////////////////// line numbers //////////////////////////////////

                    u32 line_offset = column_numbers_size + 15;
                    for(char* c = text_editor.lines[line_index].line; *c != '\0'; c++) {
                        LoadedGlyph glyph = font_table[(u32)*c];
                        // TODO `y_baseline` is fine, but what is NOT fine is `y_baseline * (line_index + 1)` because the height is based on the font size
                        draw_bitmap(&global_buffer, line_offset, y_baseline * (line_index + 1), glyph);
                        line_offset += glyph.advance_x >> 6;
                    }
                }
                //char linesssss[20][1000];
                //const char* line = "hola";
                //const char** lines = {"hola", "como", "estas?"}; // doesnt' compile
                //const char*** liness = [3]["hola", "como", "estas?"]; // doesnt' compile

                // cuando estoy editando. Tengo todo el archivo en memoria hasta guardarlo? Seguramente 

                // curr_y_baseline
                // curr_line
                // curr_col

                // lines grow until '\n' or '\0'
                // const char** line (array of characters "["hola", "como", "estas??"])




                // UI
                {

                    UI_Button buttons[10];
                    i32 button_count = 10;
                    // add buttons
                    for(i32 i = 0; i < button_count; i++) {
                        UI_Button button = {0};
                        button.ui_flags |= UI_Button_Flags_Hover;
                        button.x = 100 + i * 60;
                        button.y = 100;
                        button.w = 50;
                        button.h = 50;
                        button.color = 0xFF00FFFF;
                        button.text = "Click me!";
                        buttons[i] = button;
                    }

                    // draw ui
                    for(i32 i = 0; i < button_count; i++) {
                        draw_button(&global_buffer, buttons[i]);
                    }
                }

                // draw
                OS_Window_Dimension win_dim = os_win32_get_window_dimension(window.handle);
                HDC device_context = GetDC(window.handle);
                os_win32_display_buffer(device_context, &global_buffer, win_dim.width, win_dim.height);
                ReleaseDC(window.handle, device_context);
            }
        }
        

        if (global_toggle_fullscreen) {
            os_win32_toggle_fullscreen(window.handle);
            global_toggle_fullscreen = false;
        }

        global_input.prev_keyboard_state = global_input.curr_keyboard_state;
    }


    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    size_t windows_page_size = info.dwPageSize;
    char buf[200];
    sprintf(buf,  "Windows page size: %zd\n", windows_page_size);
    OutputDebugStringA(buf);

    printf("Windows large page size: %zd\n",GetLargePageMinimum());

    size_t def_commit_size = 1024 * 64;
    printf("Commit size: %zd\n", def_commit_size);
    size_t new_commit_size = align_pow2(def_commit_size, 4 * 1024);
    printf("Commit size: %zd\n", new_commit_size);

    TempArena temp_arena1 = temp_begin(&global_arena);

    u8* bytes = some(temp_arena1.arena);
    for(int i = 0; i < 10; i++) {
        printf("%c, ", bytes[i]);
    }
    printf("\n");

    // What happens is that this values reads the previous values stored in the temp arena
    // created in `some` and because the pointer is shared between arenas I can use the original arena up until I reached
    // the point where the temp arena (no longer availabe) was created. 
    // DONE do it in ginger code
    printf("First rubbish \n");
    u8* rubbish = arena_push_size(&global_arena, u8, 10);
    for(int i = 0; i < 10; i++) {
        printf("%c, ", rubbish[i]);
    }
    printf("\n");

    printf("Second rubbish \n");
    rubbish = arena_push_size(&global_arena, u8, 10);
    for(int i = 0; i < 10; i++) {
        printf("%c, ", rubbish[i]);
    }
    printf("\n");

    //TODO Add this
    printf("bytes again \n");
    for(int i = 0; i < 10; i++) {
        printf("%c, ", bytes[i]);
    }
    printf("\n");

    temp_end(temp_arena1);


    //OS_ReadFileResult font_file = os_read_file(temp_arena.arena, "C:\\Windows\\Fonts\\Arial.ttf");


    // Datastructures
    /* TODOs
        - Lists and list macros 
        - Metaprogramming or parsing
    */
    {
        void* memory = malloc(mb(10));
        typedef u32 NodeFlags;
        enum {
            NodeFlag_MaskSetDelimiters          = (0x3F<<0),
            NodeFlag_HasParenLeft               = (1<<0),
            NodeFlag_HasParenRight              = (1<<1),
            NodeFlag_HasBracketLeft             = (1<<2),
            NodeFlag_HasBracketRight            = (1<<3),
            NodeFlag_HasBraceLeft               = (1<<4),
            NodeFlag_HasBraceRight              = (1<<5),
            
            NodeFlag_MaskSeparators             = (0xF<<6),
            NodeFlag_IsBeforeSemicolon          = (1<<6),
            NodeFlag_IsAfterSemicolon           = (1<<7),
            NodeFlag_IsBeforeComma              = (1<<8),
            NodeFlag_IsAfterComma               = (1<<9),
        };

        enum NodeKind {
            NodeKind_Nil,
            NodeKind_File,
            NodeKind_Tag,
            NodeKind_List,
            NodeKind_COUNT
        };

        enum MsgKind {
            MsgKind_Null,
            MsgKind_Note,
            MsgKind_Warning,
            MsgKind_Error,
            MsgKind_COUNT
        };

        struct Node {
            // tree links
            Node* first;
            Node* last;
            Node* parent;
            Node* prev;
            Node* next;

            // node info
            NodeFlags flags;
        };

        struct Msg {
            Msg* next;
            Node* node;
            Str8 string;
            NodeKind kind;
        };

        struct MsgList {
            Msg* first;
            Msg* last;
            u32 count;
        };

    }
}

void EntryPoint() {

}