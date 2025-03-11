#define AIM_PROFILER 1
#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H


#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "base/base_os.h"
#include "aim_profiler.h"

#include "base/base_string.cpp"
#include "base/base_arena.cpp"
#include "base/base_os.cpp"
#include "aim_profiler.cpp"
#include "aim_timer.cpp"

#include <psapi.h>
/*
    - Maybe its good that lines know where they are, their line number. Because when I want to add a newline in the middle of many lines, i can just update all lines with its new line and not do a copy

*/

union FlagsUnion {
    struct {
        u32 pressed_left   : 1;
        u32 pressed_middle : 1;
        u32 pressed_right  : 1;
    };
    u32 all_flags;
};



global_variable int window_width = 1280;
global_variable int window_height = 720;
global_variable i32 max_glyph_width;
global_variable i32 max_char_width;
global_variable i32 max_glyph_height;
global_variable i32 ascent;    // Distance from baseline to top (positive)
global_variable i32 descent; // Distance from baseline to bottom (positive)
global_variable i32 line_height;

global_variable FT_Library library;
global_variable Arena global_arena;
global_variable Arena global_scratch_arena;

global_variable char* global_line;
global_variable u32 global_cursor_in_line;
global_variable u32 global_curr_line_number = 0;
global_variable u32 global_max_line_length = 300;


struct LoadedGlyph;

enum CursorType
{
    CursorType_Normal,
    CursorType_Insert,
};

/*
    Should I have two cursors, one for editing and one for clicking around?
    Should I have one Cursor but with x, y instead of line and column

*/
struct Cursor {
    u32 line { 1 };
    u32 column { 1 };
    u32 scroll_x {0};
    u32 scroll_y {0};
    LoadedGlyph* curr_glyph;
};

struct Line {
    char* line;
    u32 max_char_count;
};

struct TextEditor {
    CursorType edition_mode;
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

struct UI_State
{
   b32 primary_menu_opened;
};

global_variable UI_State ui_state;
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

u32 click_left(Input* input) {
    u32 result = 0;
    result = input->curr_mouse_state.button[Buttons_LeftClick] == 1;
    return result;
}

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
    result = (
        is_key_pressed(input, Keys_Shift) ||
        is_key_pressed(input, Keys_ShiftLeft) ||
        is_key_pressed(input, Keys_ShiftRight) ||
        is_key_pressed(input, Keys_ControlLeft) ||
        is_key_pressed(input, Keys_ControlRight)
    );
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
    i32 bitmap_left;
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

size_t cstring_length(char* str) {
    size_t result = 0;
    for(char* c = str; *c != '\0'; c++) {
        result++;
    }
    return result;
}

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
    result = &text_editor->lines[text_editor->cursor.line];
    return result;
}

void add_char_to_line(TextEditor* text_editor, Line* line, char ch){
    Cursor* curr_cursor = &text_editor->cursor;
    // TODO better names, remove excessive vars invoking
    if(ch == '\n')
    {
        // TODO incomplete, does not handle when there is a line below already. Only works when you press enter on the first line!
        size_t len = cstring_length(&line->line[curr_cursor->column]);

        Line* next_line = &text_editor->lines[text_editor->cursor.line + 1];
        if (next_line->max_char_count == 0)
        {
            memcpy(&next_line->line[1], &line->line[curr_cursor->column], len);
            line->line[curr_cursor->column] = ch;
            line->line[curr_cursor->column + 1] = '\0';
            next_line->max_char_count += len;
            line->max_char_count -= len;
            text_editor->cursor.line++;
            text_editor->cursor.column = 1;
            text_editor->line_count++;
        }
        else
        {
            /*
                memset(next_line.line, 0, next_line.max_char_count);
                next_line.max_char_count = 0; 
            */
            Line* next_next_line = &text_editor->lines[text_editor->cursor.line + 2];

            memcpy(&next_next_line->line[1], &next_line->line[1], next_line->max_char_count + 1);
            memcpy(&next_line->line[1], &line->line[curr_cursor->column], len);
            next_next_line->max_char_count = next_line->max_char_count;
            next_line->max_char_count += len; 
            memset(&line->line[curr_cursor->column], 0, len);
            line->max_char_count -= len;


            text_editor->cursor.line++;
            text_editor->cursor.column = 1;
            text_editor->line_count++;


            // same as above!
            //memcpy(&new_line->line[1], &line->line[curr_cursor->column], len);
            //line->line[curr_cursor->column] = ch;
            //line->line[curr_cursor->column + 1] = '\0';
            //new_line->max_char_count += len;
            //line->max_char_count -= len;
            //text_editor->cursor.line++;
            //text_editor->cursor.column = 1;
            //text_editor->line_count++;
            
        }
    }
    else
    {

        size_t len = cstring_length(&line->line[curr_cursor->column]);
        memcpy(&line->line[curr_cursor->column + 1], &line->line[curr_cursor->column], len);
        line->line[curr_cursor->column]  = ch;
        curr_cursor->column++;
        line->max_char_count++;
        text_editor->cursor.curr_glyph = &font_table[u32(ch)];

        //line->line[curr_cursor->column]  = ch;
        //curr_cursor->column++;
        //line->max_char_count++;
        //text_editor->cursor.curr_glyph = &font_table[u32(ch)];
    }

}

// TODO esta logica esta flawed
// Ver mas adelante cuando pruebe con palaabras de 4 caracteres, etc como "hola"
// Ver cuando pase a usar otro cursor
// Cuando esta arriba de la letra en bloque entonces tiene la pos de la letra, cuando estoy en insert tiene la pos siguiente!!!
// TODO use bitmap_left to offset the bitmap like in the tutorial 
u32 remove_char_from_line(TextEditor* text_editor, Line* line, u32 rel_line_pos, u32 count = 1, char replace_by = char(32)) {
    u32 pos = rel_line_pos;
    for(u32 i = 0; i < count; i++) {
        pos--;
        line->line[pos] = replace_by;
    }
    pos--;

    text_editor->cursor.column -= count;
    line->max_char_count -= count;
    // Lo mas importante de esto era tener bien esta logica!
    text_editor->cursor.curr_glyph = &font_table[u32(line->line[pos])];

    return count;
}

void reset_current_cursor(TextEditor* text_editor) {
    text_editor->cursor.column = 1;
    text_editor->cursor.scroll_x = 0;
    text_editor->cursor.scroll_y = 0;
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
                global_os_w32_window.is_running = false;
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
                if (text_editor.edition_mode == CursorType_Insert)
                {
                    // NOTE This only works for sequential insertion. The moment i place the cursor anywhere on the line and enter chars it will bypass this check
                    // This is only here so to avoid crashes. Delay until I know what to do with infinite lines!
                    if(text_editor.cursor.column < global_max_line_length)
                    {
                        if((ch >= ' ' && ch != 127) || ch == '\n' || ch == '\t') 
                        {
                            u32 bitmask_29 = (1 << 29);
                            Line* line = get_current_line(&text_editor);
                            add_char_to_line(&text_editor, line, ch);
                        }
                    }

                    if(is_key_pressed(&global_input, Keys_Backspace)) {
                        Line* line = get_current_line(&text_editor);
                        u32 chars_removed = remove_char_from_line(&text_editor, line, text_editor.cursor.column);
                    }

                    //if(is_key_pressed(&global_input, Keys_Enter)) {
                    //    Line* old_line = get_current_line(&text_editor);
                    //    Cursor old_cursor = text_editor.cursor;

                    //    // add new line
                    //    text_editor.cursor.line++;
                    //    text_editor.line_count++;

                    //    // TODO cuando termine esto capaz que sea mejor que get_current_line tome un cursor
                    //    Line* new_line = get_current_line(&text_editor);

                    //    u32 cpy_len = cstring_length(&old_line->line[old_cursor.column]);
                    //    memcpy(new_line->line , &old_line->line[old_cursor.column], cpy_len);
                    //    new_line->max_char_count += cpy_len;

                    //    old_line->max_char_count -= cpy_len;
                    //    memset(&old_line->line[old_cursor.column], 0, cpy_len);

                    //    reset_current_cursor(&text_editor);
                    //}
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

				if ( is_key_pressed(&global_input, Keys_Arrow_Left) ) 
				{
					if(text_editor.cursor.column > 1)
					{
						text_editor.cursor.column--;
					}
				}
				if ( is_key_pressed(&global_input, Keys_Arrow_Right)) 
				{
					// TODO this value is not what I want. I want editor width, or panel width or something like that
					if(text_editor.cursor.column <= text_editor.lines[text_editor.cursor.line].max_char_count)
					{
						text_editor.cursor.column++;

					}
				}


                /*
                        // TODO improve this logic. for now just go to the last ch in the next line


                        // TODO add this
                        DOWN - if current_line.chars > next_line.chars
                                    go to last character of next_line
                                else
                                    go to the same column im at but in the next line

                        UP - if current_line.chars > prev_line.chars
                                    go to last character of prev_line
                                else
                                    go to the same column im at but in the prev line0


                        NOTE both DOWN and UP logic are the same
                */
				if ( is_key_pressed(&global_input, Keys_Arrow_Up)) 
				{
					if(text_editor.cursor.line > 0)
                    {
						u32 prev_line_index = --text_editor.cursor.line;
                        text_editor.cursor.column = text_editor.lines[prev_line_index].max_char_count + 1;
                    }
				}

				if ( is_key_pressed(&global_input, Keys_Arrow_Down)) 
				{
					if(text_editor.cursor.line < text_editor.line_count)
                    {
						u32 next_line_index = ++text_editor.cursor.line;
                        text_editor.cursor.column = text_editor.lines[next_line_index].max_char_count + 1;
                    }
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

typedef u32 UI_ButtonFlags;
enum {
    UI_ButtonFlags_Drag = (1 << 0),
    UI_ButtonFlags_Hover = (1 << 1),
    UI_ButtonFlags_Click = (1 << 2),
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
    For this before drawing i need to do a pass inside every button and if rect_contains(a, b) break... the problem is that all the other buttons must be resseted!
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


struct Rect2D
{
    i32 x;
    i32 y;
    i32 w;
    i32 h;
};

struct Point2D
{
    i32 x;
    i32 y;
};

b32 rect_contains_point(Rect2D rect, Point2D point) {
    b32 result = ((point.x > rect.x) &&
                  (point.x < rect.x + rect.w) && 
                  (point.y > rect.y) &&
                  (point.y < rect.y + rect.h));
    return result;
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

//aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
//aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa //

global_variable f32 line_number_size = 0;
void draw_bitmap(OS_Window_Buffer* buffer, i32 x_baseline, i32 y_baseline, LoadedGlyph hdp) {
    //aim_profiler_time_function;
    u32 x = x_baseline;
    u32 y = y_baseline;
    u32 width = hdp.bitmap.width;
    u32 height = hdp.bitmap.height;

    if (x > buffer->width)
    {
        x = buffer->width;
    }

    if(x + width > buffer->width)
    {

        width = buffer->width - x;
        //scroll_x += 1;

    }

    if(y > buffer->height)
    {
        y = buffer->height;
    }

    if(y + height > buffer->height)
    {
        
        height = buffer->height - y;
    }

    i32 new_y = y_baseline - hdp.bitmap_top;

    u8* dest_row = buffer->pixels + new_y * buffer->pitch + x_baseline * 4;
    u32* dest_row2 = (u32*)buffer->pixels + new_y * buffer->width + x_baseline;
    {
        for(i32 y = 0; y < height; y++){
            u32* destrow = (u32*) dest_row2;
            u32* dest_pixel = (u32*) dest_row;
            for(i32 x = 0; x < width; x++) {
                u8* src_pixel = hdp.bitmap.buffer + width * y + x;
                u32 alpha = *src_pixel << 24;
                u32 red = *src_pixel << 16;
                u32 green = *src_pixel << 8;
                u32 blue = *src_pixel;
                //*dest_pixel++ = color;

                //u32* sourcerow = (u32*)hdp.bitmap.buffer + hdp.bitmap.width * y + x;
                //f32 sa = (f32)((*sourcerow >> 24) & 0xFF) / 255.0f;
                //f32 sr = (f32)((*sourcerow >> 16) & 0xFF);
                //f32 sg = (f32)((*sourcerow >> 8) & 0xFF);
                //f32 sb = (f32)((*sourcerow >> 0) & 0xFF);

                //f32 dr = (f32)((*destrow >> 16) & 0xFF);
                //f32 dg = (f32)((*destrow >> 8) & 0xFF);
                //f32 db = (f32)((*destrow >> 0) & 0xFF);

                //// Blend equation: linear interpolation (lerp)
                //u32 nr = u32((1.0f - sa) * dr + sa*sr);
                //u32 ng = u32((1.0f - sa) * dg + sa*sg);
                //u32 nb = u32((1.0f - sa) * db + sa*sb);

                //*destrow = (nr << 16) | (ng << 8) | (nb << 0);
                //destrow++;


                // alright so dest is the background and in this case the dest_row (buffer->memory)
                // formula: alpha * src + (1 - alpha) * dest
                // => (1.0f - t) * A + t * B
                // => A-tA + tB
                // => A+t(B - A)

                f32 sa = (f32)(*src_pixel / 255.0f);

                u32 color2 = 0xFFFFFFFF;

                f32 sr = (f32)((color2 >> 16) & 0xFF);
                f32 sg = (f32)((color2 >> 8) & 0xFF);
                f32 sb = (f32)((color2 >> 0) & 0xFF);

                f32 dr = (f32)((*destrow >> 16) & 0xFF);
                f32 dg = (f32)((*destrow >> 8) & 0xFF);
                f32 db = (f32)((*destrow >> 0) & 0xFF);
                u32 nr = u32((1.0f - sa) * dr + sa*sr);
                u32 ng = u32((1.0f - sa) * dg + sa*sg);
                u32 nb = u32((1.0f - sa) * db + sa*sb);

                *destrow = (nr << 16) | (ng << 8) | (nb << 0);
                //*destrow = red;
                destrow++;

            }
            dest_row += buffer->pitch;
            dest_row2 += buffer->width;
        }

    }
}

union ButtonState {
    struct {
        u32 pressed_left   : 1;
        u32 pressed_middle : 1;
        u32 pressed_right  : 1;
    };
    u32 all_flags;
};

struct UI_Widget
{
    i32 x;
    i32 y;
    i32 w;
    i32 h;
    u32 color;
    const char* text;
    ButtonState clicked_state;
    u32 flags;
    u32 hovered;
};



UI_Widget make_widget(i32 x, i32 y, i32 w, i32 h, u32 color, const char* text = 0) 
{
    UI_Widget result = {0};
    result.flags |= UI_ButtonFlags_Hover;

    result.x = x;
    result.y = y;
    result.w = w;
    result.h = h;
    result.color = color;
    result.text = text;
    if (rect_contains_point({x, y, w, h}, {global_input.curr_mouse_state.x, global_input.curr_mouse_state.y})) 
    {
        if(click_left(&global_input))
        {
            result.clicked_state.pressed_left = 1;
        }
        if(result.flags & UI_ButtonFlags_Hover) {
            result.hovered = 1;
        }
    }

    return result;
}


void draw_cursor_insert_mode(OS_Window_Buffer* buffer, Cursor* cursor) {
    u32 cursor_width_px = 2.0f;

    u32 column_numbers_size = 30;
    u32 line_offset = column_numbers_size + 15;
    u32 bitmap_offset = 0;
    if (cursor->curr_glyph)
    {
        bitmap_offset = cursor->curr_glyph->bitmap_left / 2;
    }
    /*  NOTE esto era antes final_dest_x
        i32 dest_x = ((cursor->column - 1 - scroll_x) * (max_char_width >> 6) + line_offset) - bitmap_offset;

        u32 cw = max_char_width >> 6;
        i32 dest_x = (cursor->column - 1) * cw + line_offset - bitmap_offset;
        i32 final_dest_x = dest_x - scroll_x * cw;

        if (final_dest_x >= buffer->width)
        {
            scroll_x++;
            final_dest_x = final_dest_x - cw;
        }
    */

    u32 cw = max_char_width >> 6;
    i32 dest_x = (cursor->column - 1) * cw + line_offset - bitmap_offset;
    i32 final_dest_x = dest_x - cursor->scroll_x * cw;
    i32 dest_y = ((cursor->line) * line_height) - ascent;
    i32 final_dest_y = dest_y - cursor->scroll_y * cw;


    // (scroll_x + 1) * cw => scroll_x * cw + scroll_x
    if (final_dest_x >= buffer->width)
    {
        cursor->scroll_x++;
        final_dest_x = final_dest_x - cw;
        // NOTE these are here just in case i messed up the equation
        i32 final_dest_x_aux = dest_x - cursor->scroll_x * cw;
        i32 a =1;
    }

    // TODO wtf!
    if (final_dest_x < 45)
    {
        cursor->scroll_x--;
        final_dest_x = final_dest_x + cw;
        // NOTE these are here just in case i messed up the equation
        i32 final_dest_x_aux = dest_x - cursor->scroll_x * cw;
        i32 a =1;
    }

    //if (final_dest_y >= buffer->height)
    //{
    //    cursor->scroll_y++;
    //    final_dest_x = final_dest_y - cw;
    //    // NOTE these are here just in case i messed up the equation
    //    i32 final_dest_y_aux = dest_y - cursor->scroll_y * cw;
    //    i32 a =1;
    //}

    //// TODO wtf!
    //if (final_dest_y < 0)
    //{
    //    cursor->scroll_y--;
    //    final_dest_x = final_dest_y + cw;
    //    // NOTE these are here just in case i messed up the equation
    //    i32 final_dest_y_aux = dest_y - cursor->scroll_y * cw;
    //    i32 a =1;
    //}



    {
        char buf[200];
        sprintf(buf, "dest_x: %d, dest_y: %d\n", final_dest_x, final_dest_y);
        u32 line_offset = 200;
        u32 y_baseline2 = 700;
        for(char* c = buf; *c != '\0'; c++)
        {
            LoadedGlyph glyph = font_table[(u32)*c];
            // TODO `y_baseline` is fine, but what is NOT fine is `y_baseline * (line_index + 1)` because the height is based on the font size
            draw_bitmap(&global_buffer, glyph.bitmap_left + line_offset, y_baseline2, glyph);
            line_offset += glyph.advance_x >> 6;
        }
    }
    u8* row = buffer->pixels + buffer->pitch * final_dest_y + final_dest_x * 4;

    for(i32 y = 0; y < line_height; y++) {
        u32* pixel = (u32*)row;
        for(i32 x = 0; x < cursor_width_px; x++) {
            // ARGB -> 0xAARRGGBB
            //*pixel++ = 0xFfFFFF00;
            *pixel++ = 0xFf0000ff;
        }
        row += buffer->pitch;
    }
}

void draw_cursor_normal_mode(OS_Window_Buffer* buffer, Cursor* cursor) {
    u32 column_numbers_size = 30;
    u32 line_offset = column_numbers_size + 15;
    u32 bitmap_offset = 0;
    if (cursor->curr_glyph)
    {
        bitmap_offset = cursor->curr_glyph->bitmap_left / 2;
    }
    i32 dest_x = ((cursor->column - 1) * ((max_char_width >> 6)) + line_offset) - bitmap_offset;
    i32 dest_y = ((cursor->line) * line_height) - ascent;
    u8* row = buffer->pixels + buffer->pitch * dest_y + dest_x * 4;

    for(i32 y = 0; y < ascent + descent; y++) {
        u32* pixel = (u32*)row;
        for(i32 x = 0; x < max_char_width >> 6; x++) {
            // ARGB -> 0xAARRGGBB
            //*pixel++ = 0xFfFFFF00;
            *pixel++ = 0xFf0000ff;
        }
        row += buffer->pitch;
    }
}

void draw_cursor(OS_Window_Buffer* buffer, Cursor* cursor, CursorType type)
{
    switch(type)
    {
        case CursorType_Normal: 
        {
            draw_cursor_normal_mode(buffer, cursor);
        } break;
        case CursorType_Insert: 
        {
            draw_cursor_insert_mode(buffer, cursor);
        } break;
    }
};

void draw_widget(OS_Window_Buffer* buffer, UI_Widget button) {
    int dest_x = button.x;
    int dest_y = button.y;
    int height = button.h;
    int width = button.w;
    int color = button.color;

    //if(button.ui_flags & UI_ButtonFlags_Hover) {
    //    if (rect_contains_point({dest_x, dest_y, width, height}, {global_input.curr_mouse_state.x, global_input.curr_mouse_state.y})) {
    //        color = hovered_color;
    //    }
    //}

    u8* row = buffer->pixels + buffer->pitch * dest_y + dest_x * 4;
    for(i32 y = 0; y < height; y++) {
        u32* pixel = (u32*)row;
        for(i32 x = 0; x < width; x++) {
            *pixel++ = color;
        }
        row += buffer->pitch;
    }

    u32 line_offset = 0;
    if (button.text) 
    {
        for(const char* c = button.text; *c; c++) {
            LoadedGlyph glyph = font_table[u32(*c)];
            draw_bitmap(buffer, line_offset + button.x + button.w / 2, button.y + button.h / 2, glyph);
            line_offset += glyph.advance_x >> 6;
        }
    }
}

void draw_button(OS_Window_Buffer* buffer, UI_Button button) {
    int dest_x = button.x;
    int dest_y = button.y;
    int height = button.h;
    int width = button.w;
    int color = button.color;
    int hovered_color = 0xFFFFFFFF;

    if(button.ui_flags & UI_ButtonFlags_Hover) {

        if (rect_contains_point({dest_x, dest_y, width, height}, {global_input.curr_mouse_state.x, global_input.curr_mouse_state.y})) {
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

    u32 line_offset = 0;
    for(const char* c = button.text; *c; c++) {
        LoadedGlyph glyph = font_table[u32(*c)];
        draw_bitmap(buffer, line_offset + button.x + button.w / 2, button.y + button.h / 2, glyph);
        line_offset += glyph.advance_x >> 6;
    }
}



LoadedGlyph load_glyph(FT_Face face, char codepoint) {
    LoadedGlyph result = {0};
    // The index has nothing to do with the codepoint
    u32 glyph_index = FT_Get_Char_Index(face, codepoint);
    if (glyph_index) 
    {
        FT_Error load_glyph_err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT); 
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

		max_glyph_width = max(max_glyph_width, face->glyph->bitmap.width);
		max_glyph_height = max(max_glyph_height, face->glyph->bitmap.rows);

        result.bitmap.width = face->glyph->bitmap.width;
        result.bitmap.height = face->glyph->bitmap.rows;
        result.bitmap.pitch = face->glyph->bitmap.pitch;
        
        result.bitmap_top = face->glyph->bitmap_top;
        result.bitmap_left = face->glyph->bitmap_left;
        result.advance_x = face->glyph->advance.x;
        max_char_width = max(max_char_width, result.advance_x);

        FT_Bitmap* bitmap = &face->glyph->bitmap;
        result.bitmap.buffer = (u8*)malloc(result.bitmap.pitch * result.bitmap.height);
        memcpy(result.bitmap.buffer, bitmap->buffer, result.bitmap.pitch * result.bitmap.height);


    }
    return result;
}



struct CmdLine {

};

void EntryPoint();

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode) {
     AllocConsole();

    FILE* fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    aim_profiler_begin();
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
    size_t arena_total_size = mb(1);

    arena_init(&global_arena, arena_total_size);
    arena_init(&global_scratch_arena, arena_total_size);

    FT_Error error = FT_Init_FreeType(&library);
    if (error)
    {
        const char* err_str = FT_Error_String(error);
        printf("FT_Init_FreeType: %s\n", err_str);
        exit(1);
    }


    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    RECT window_rect = {0};
    SetRect(&window_rect,
    (screenWidth / 2) - (window_width / 2),
    (screenHeight / 2) - (window_height / 2),
    (screenWidth / 2) + (window_width / 2),
    (screenHeight / 2) + (window_height / 2));
    DWORD style = (WS_OVERLAPPED | WS_VISIBLE);
    //window_rect.right = window_width;
    //window_rect.bottom = window_height;
    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, false);
    OS_Window window = os_win32_open_window(window_rect);

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
        //OS_FileReadResult font_file = os_file_read(&global_arena, "C:\\Windows\\Fonts\\Arial.ttf");
        OS_FileReadResult font_file = os_file_read(&global_arena, "C:\\Windows\\Fonts\\CascadiaMono.ttf");

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
            //max_glyph_width = face->size->metrics.max_advance;
            //max_glyph_height = face->size->metrics.height;
			ascent = face->size->metrics.ascender >> 6;    // Distance from baseline to top (positive)
			descent = - (face->size->metrics.descender >> 6); // Distance from baseline to bottom (positive)
			line_height = face->size->metrics.height >> 6;

            max_glyph_width = 0;
            max_glyph_height = 0;


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


                // * - "The metrics found in face->glyph->metrics are normally expressed in 26.6 pixel format"
            }

            {
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

            {
                aim_profiler_time_block("font_table creation")
                for(u32 codepoint = '!'; codepoint <= '~'; codepoint++) {
                    font_table[u32(codepoint)] = load_glyph(face, char(codepoint));
                }
                font_table[u32(' ')] = load_glyph(face, char(' '));
            }

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
        global_os_w32_window.is_running = true;
        while(global_os_w32_window.is_running) 
        {

            POINT mouse_point;
            DWORD LastError;
            if (GetCursorPos(&mouse_point) && ScreenToClient(global_os_w32_window.handle, &mouse_point))
            {
                int fjdksal = 1231;
            }
            if (GetCursorPos(&mouse_point) && ScreenToClient(global_os_w32_window.handle, &mouse_point))
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
            //draw_rect(&global_buffer, 0, 0, global_buffer.width, global_buffer.height, 0xFFffffff);

            

            {
                // Text drawing routine
                {
                    if(text_editor.line_count > 0 && text_editor.cursor.column > 0)
                    {
                        draw_cursor(&global_buffer, &text_editor.cursor, text_editor.edition_mode);
                    }

                    // editor
                    i32 y_baseline = line_height;

                    char* word = "Sale Dota eeh!";
                    word = "The sound of ocean waves calms my soul";
                    word = "The sound of the ocean";
                    word = "GameUpdateAndRender?? hola";
                    //for(char* c = global_line; *c != '\0'; c++) {
                    u32 last_line_index = text_editor.line_count;

                    // line_number = 120;
                    // char number = "120";
                    aim_profiler_time_block("Text and line numbers routine");
                    for(u32 line_index = 1 - text_editor.cursor.scroll_y; line_index <= last_line_index; line_index++) {
                        i32 column_numbers_size = 30;
                        // TODO this `init_pos` should go away when I have the parent sorted.

                        ///////////////////////////// line numbers //////////////////////////////////
                        /* 
                        NOTE line numbers in vim: it has a size for the column numbers that grows based on the total line_count of the file

                        */
                        // NOTE instead of a hash I could have used: char* number_to_str[1000000000] = 
                        {
                            aim_profiler_time_block("Line numbers drawing");
                            u32 index = line_numbers_to_chars_hash(line_index ) % array_count(line_numbers_to_chars);
                            LineNumberToChars* line_in_chars = get_line_number_in_chars(line_numbers_to_chars, index, line_index );
                            char* buf = (char*)malloc(10);
                            if(line_in_chars) {
                                buf = line_in_chars->buf;
                            } else {
                                itoa(line_index , buf, 10);
                                add_line_number_in_chars(line_numbers_to_chars, index, line_index , buf);
                            }
                            size_t buf_size = cstring_length(buf);
                            u32 char_offset = 0;
                            u32 pos = 0;
                            for(i32 i = buf_size - 1; i >= 0; i--, pos++) {
                                LoadedGlyph glyph = font_table[(u32)buf[i]];
                                char_offset += glyph.advance_x >> 6;
                                draw_bitmap(&global_buffer, glyph.bitmap_left + column_numbers_size - char_offset, y_baseline * (line_index), glyph);
                            }
                        }

                        {
                            /////////////////////////////    t_text    //////////////////////////////////
                            aim_profiler_time_block("Text Drawing");
                            u32 line_offset = column_numbers_size + 15;
                            line_number_size = line_offset;
                            char* current_line = text_editor.lines[line_index].line + 1 + text_editor.cursor.scroll_x;
                            for(char* c = current_line; *c != '\0' && *c != '\n'; c++) {
                                LoadedGlyph glyph = font_table[(u32)*c];
                                // TODO `y_baseline` is fine, but what is NOT fine is `y_baseline * (line_index + 1)` because the height is based on the font size
                                draw_bitmap(&global_buffer,  glyph.bitmap_left + line_offset, y_baseline * line_index, glyph);
                                line_offset += glyph.advance_x >> 6;
                            }
                        }
                    }
                
                }


                {
                    ///////////////////// print line info ////////////////////////////////
                    char buf[200];
                    sprintf(buf, "Line number: %d, line column: %d, max_line_ch_cnt: %d\n", text_editor.cursor.line, text_editor.cursor.column, text_editor.lines[text_editor.cursor.line].max_char_count);
                    u32 line_offset = 700;
                    u32 y_baseline2 = 700;
                    for(char* c = buf; *c != '\0'; c++)
                    {
                        LoadedGlyph glyph = font_table[(u32)*c];
                        // TODO `y_baseline` is fine, but what is NOT fine is `y_baseline * (line_index + 1)` because the height is based on the font size
                        draw_bitmap(&global_buffer, glyph.bitmap_left + line_offset, y_baseline2, glyph);
                        line_offset += glyph.advance_x >> 6;
                    }
                }

                {
                    ///////////////////// print line info ////////////////////////////////
                    char buf[200];
                    sprintf(buf, "buffer width: %d, buffer height: %d\n", global_buffer.width, global_buffer.height);
                    u32 line_offset = 100;
                    u32 y_baseline2 = 600;
                    for(char* c = buf; *c != '\0'; c++)
                    {
                        LoadedGlyph glyph = font_table[(u32)*c];
                        draw_bitmap(&global_buffer, glyph.bitmap_left + line_offset, y_baseline2, glyph);
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
                /* puedo hacer todo el build de la UI aca, no hace falta que haga otra cosa diferente sinceramente

                        Si quiero hacer esto entonces tengo que hacer 2 pases, uno donde guardo el estado de todos los botones y otro donde los dibujo
                        if(buttons[i].is_left_clicked) {

                        }

                 rad tiene UI_Signal por cada box, y cada box es una estructura compleja
                */

                {

                    UI_Button buttons[10];
                    i32 button_count = 10;

                    /*
                    This approach infers that `buttons` will persist, effectively creating a retained-mode gui
                    */
                    /*
                    for(i32 i = 0; i < button_count; i++) {
                        UI_Button button = {0};
                        button.ui_flags |= UI_ButtonFlags_Hover;
                        button.x = 300;
                        button.y = 100 + i * 52;
                        button.w = 700;
                        button.h = 50;
                        button.color = 0xFF001F00;
                        button.text = "Click me!";
                        buttons[i] = button;
                    }

                    for(i32 i = 0; i < button_count; i++) {
                        draw_button(&global_buffer, buttons[i]);
                    }
                    */
                    // TODO provisory!!!
                    ui_state.primary_menu_opened = 1;

                    /*
                        Here the buttons are created on the fly, similar to IMGUI. The entire button and its state is reconstructed each frame! 
                    */

                    const char* buttons_text[10] = {
                        "Button 1",
                        "Button 2",
                        "Button 3",
                        "Button 4",
                        "Button 5",
                        "Button 6",
                        "Button 6",
                        "Button 8",
                        "Button 9",
                        "Button 10",
                    };
                    ////////////////////

                    if (text_editor.edition_mode == CursorType_Insert)
                    {
                        if (is_key_pressed(&global_input, Keys_ESC))
                        {
                            text_editor.edition_mode = CursorType_Normal;
                        }
                    }
                    else
                    {
                        if (text_editor.edition_mode == CursorType_Normal)
                        {
                            if (is_key_pressed(&global_input, Keys_I))
                            {
                                text_editor.edition_mode = CursorType_Insert;
                            }
                        }
                    }


                    /* 
                    NOTE
                        Esto es tricky porque es transient pero en varios frames.
                        Siempre vi que tienen dos arenas, una para siempre y otra para data que se borra por cada frame. En este caso no es asi. Como esto va a persistir
                        hasta que cierre el menu. Tengo que sacar la temp de una arena que vive siempre. Por eso la saque de global_arena. Aunque tambien
                        podria sacarla de esa arena scratch global. Que se usa para este tipo de cosas, siempre y cuando no la borre no pasa nada. Supongo que para este
                        caso cualqueira de las dos cosas va. No entiendo bien como es la onda con esto la verdad!
                        Voy a hacer los dos casos! Primero hago la temp desde la global persistent arena y despues de la global transient arena
                        Igual seguro ni calienta esto porque todos hacen pura mierda todo el tiempo
                    */ 
                    TempArena arena_input_buffer;
                    if ( is_key_pressed(&global_input, Keys_O) && os_modifiers & OS_Modifiers_Ctrl && !(os_modifiers & OS_Modifiers_Shift) &&
                        !ui_state.primary_menu_opened) 
                    {
                        arena_input_buffer = temp_begin(&global_arena);
                        ui_state.primary_menu_opened = 1;
                    }

                    if (is_key_pressed(&global_input, Keys_ESC) && ui_state.primary_menu_opened) 
                    {
                        ui_state.primary_menu_opened = 0;
                        temp_end(arena_input_buffer);
                    }

                    if(ui_state.primary_menu_opened) 
                    {
                        // TODO push this menu onto a stack?

                        // TODO turn this into a list!
                        UI_Widget widgets[11];
                        u32 left_clicked_color = 0xFFF11F12F;
                        u32 hovered_color = 0xFFF1F1F1;
                        i32 element_height = 45; 
                        i32 menu_width = 300;
                        Rect2D menu_dim = {(window_width / 2) - (menu_width / 2), 80, menu_width, 500};
                        u32 padding = 5;
                        u32 curr_pos_x = menu_dim.x;
                        u32 curr_pos_y = menu_dim.y;
                        for(u32 i = 0; i < 10; i++) 
                        {
                            u32 color = 0xFFF01FFF;
                            UI_Widget widget = make_widget(curr_pos_x, curr_pos_y, menu_dim.w, element_height, color, buttons_text[i]);
                            if (widget.hovered) {
                                widget.color = hovered_color;
                            }
                            if (widget.clicked_state.pressed_left) {
                                widget.color = left_clicked_color;
                            }
                            widgets[i] = widget;
                            curr_pos_y += element_height + padding;
                        }
                        // make textview
                        // TODO When I click the text view I must set the focus of the application to that text view
                        // draw the cursor, and start writing there. So I need another data structure for text, for this specific text view in this specific execution
                        // until I unfocus it!  This is pretty interesting! 
                        // TODO when i start searching I need to decrease the drew text while shrinking the menu area!
                        // TODO Do I need a flag when making the widget that specifies its a text view?

                        // NOTE I will start handling clicking the text view and start writing text into it
                        u32 color = 0xFFFF0000;
                        UI_Widget text_view = make_widget(curr_pos_x, curr_pos_y + 10, menu_dim.w, element_height, color, 0);
                        widgets[10] = text_view;
                        if (text_view.clicked_state.pressed_left) {
                            // get focus here
                            // COMPLEX draw cursor where the focus is                            
                            /*
                                The cursor design is not as good as I thought!
                                Right now it stores the line and where in the line it is. Problem is that I would like to have a Cursor when clicking this textview
                                but if I use the actual Cursor I have to specify the line number. But this line number is relative to the textview and it doesnt
                                have to align with the line on the text editor.

                                For this cursor I dont care about the concept of a line

                                Yes I need 2 cursors!

                                struct Widget {
                                    type: UI_Type_Textview
                                    ...
                                };
                            */

                        }


                        // TODO This is probably better done elsewhere at the end of the UI processing code!
                        for(u32 i = 0; i < array_count(widgets); i++) 
                        {
                            draw_widget(&global_buffer, widgets[i]);
                        }
                    }
                }

                // draw
                OS_Window_Dimension win_dim = os_win32_get_window_dimension(window.handle);
                HDC device_context = GetDC(window.handle);
                os_win32_display_buffer(device_context, &global_buffer, win_dim.width, win_dim.height);
                ReleaseDC(window.handle, device_context);
            }
        }
        

        if (global_os_w32_window.is_fullscreen) {
            os_win32_toggle_fullscreen(window.handle);
            global_os_w32_window.is_fullscreen = false;
        }

        global_input.prev_keyboard_state = global_input.curr_keyboard_state;
        global_input.prev_mouse_state = global_input.curr_mouse_state;
        //memset(&global_input.curr_mouse_state, 0, sizeof(global_input.curr_mouse_state));

    }

    // --- ProcessMemoryInfo ---
     
    HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, GetCurrentProcessId());
    PROCESS_MEMORY_COUNTERS_EX pmc = {0};

    if(GetProcessMemoryInfo(process_handle, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(PROCESS_MEMORY_COUNTERS_EX))) {
        char buf[200];
        sprintf(buf, "Page fault count: %d\n", pmc.PageFaultCount);
        OutputDebugStringA(buf);
    }

    // --- system info --
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

    aim_profiler_end();
    aim_profiler_print();
    system("pause");
}

void EntryPoint() {

}