#include "stdio.h"
#include "base/base_core.h"
#include "base/base_arena.h"
#include "base/base_string.h"
#include "os/os_core.h"
#include "draw/draw.h"
#include "font/font.h"
#include "input/input.h"

#include "base/base_arena.cpp"
#include "base/base_string.cpp"
#include "os/os_core.cpp"
#include "draw/draw.cpp"
#include "font/font.cpp"


global_variable OS_Window global_w32_window;
global_variable OS_PixelBuffer global_pixel_buffer;

///////////////// input //////////////////////
global_variable u32 os_modifiers;
global_variable Input global_input;
///////////////// input //////////////////////

struct Mat4x4
{
    f32 m[4][4];
    Mat4x4(
        f32 m00, f32 m01, f32 m02, f32 m03,
        f32 m10, f32 m11, f32 m12, f32 m13,
        f32 m20, f32 m21, f32 m22, f32 m23,
        f32 m30, f32 m31, f32 m32, f32 m33
    ) {
        m[0][0] = m00; 
        m[0][1] = m10; 
        m[0][2] = m20; 
        m[0][3] = m30; 

        m[1][0] = m01; 
        m[1][1] = m11; 
        m[1][2] = m21; 
        m[1][3] = m31; 

        m[2][0] = m02; 
        m[2][1] = m12; 
        m[2][2] = m22; 
        m[2][3] = m32; 

        m[3][0] = m03; 
        m[3][1] = m13; 
        m[3][2] = m23; 
        m[3][3] = m33; 
    };
};


b32 rect_contains_point(Rect2D rect, Point2D point) {
    b32 result = ((point.x > rect.x) &&
                  (point.x < rect.x + rect.w) && 
                  (point.y > rect.y) &&
                  (point.y < rect.y + rect.h));
    return result;
}

enum DebugInteractionType
{
    DebugInteractionType_None,
    DebugInteractionType_Toggle,
    DebugInteractionType_DragHierarchy,
};

enum DebugVariableType
{
    DebugVariableType_b32,
    DebugVariableType_u32,
    DebugVariableType_Str8,
    DebugVariableType_f32,
    DebugVariableType_Mat4x4,
    DebugVariableType_Button,
    DebugVariableType_Group,
};

struct DebugVariable;

struct DebugButton
{
    u32 color = 0xFFFFFFFF;
    u32 colors[2];
};

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
        Mat4x4 mat4x4_value;
        DebugGroup group;
        DebugButton button;
    };
};

struct DebugHierarchyNode
{
    f32 AtX;
    f32 AtY;
    DebugVariable *root_var;
    DebugHierarchyNode *parent;
    DebugHierarchyNode *next;
};

struct DebugHierarchy
{
    DebugHierarchyNode *first;
    DebugHierarchyNode *last;
};

struct DebugState
{
    Arena arena;
    FontInfo font_info;
    Point2D mouse_p;
    Point2D dmouse_p;
    u32 tab_size;

    DebugHierarchy root_hierarchy;

    DebugHierarchyNode *active_hierarchy;

    DebugHierarchyNode *hot_hierarchy;
    DebugHierarchyNode *next_hot_hierarchy;
    DebugVariable *active_variable;
    DebugVariable *hot_variable;
    DebugVariable *next_hot_variable;

    DebugInteractionType active_interaction;
    DebugInteractionType hot_interaction;
    DebugInteractionType next_hot_interaction;

};

struct DebugContext
{
    DebugState *state;
    DebugVariable *curr_group;
    DebugHierarchyNode *curr_hierarchy;
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
    
    var->parent = context->curr_group;
    var->name = name;
    var->type = type;
    if(type == DebugVariableType_Button)
    {
        var->button.colors[0] = 0xFFFF0000;
        var->button.colors[1] = 0xFFFFFFFF;
    }

    DebugVariable *group = context->curr_group;
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

internal DebugHierarchyNode *
debug_begin_hierarchy(DebugContext *debug_context, f32 x, f32 y)
{
    DebugHierarchyNode *node = arena_push_size(&debug_context->state->arena, DebugHierarchyNode, 1);
    DebugVariable *root_var = arena_push_size(&debug_context->state->arena, DebugVariable, 1);
    node->AtX = x;
    node->AtY = y;
    node->root_var = root_var;

    //debug_context->curr_group = debug_state->root_hierarchy.first->root_var;

    node->parent = debug_context->curr_hierarchy;
    debug_context->curr_hierarchy = node;

    debug_context->curr_group = debug_context->curr_hierarchy->root_var;

    DebugHierarchy *hierarchy = &debug_context->state->root_hierarchy;
    if (hierarchy->last == 0)
    {
        hierarchy->last = hierarchy->first = node;
    }
    else
    {
        hierarchy->last->next = node;
        hierarchy->last = node;
    }
    return node;
}

internal void
debug_end_hierarchy(DebugContext *debug_context)
{
    debug_context->curr_hierarchy = debug_context->curr_hierarchy->parent;
}

internal DebugVariable *
debug_begin_group(DebugContext *context, Str8 group_name)
{
    DebugVariable *var = debug_add_var(context, group_name, DebugVariableType_Group);
    var->group.expanded = true;
    context->curr_group = var;
    return var;
}


internal void 
debug_end_group(DebugContext *context)
{
    context->curr_group = context->curr_group->parent;
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
                global_input.curr_mouse_state.button[MouseButtons_LeftClick] = 0;

            } break;
            case WM_MBUTTONUP:
            {
                global_input.curr_mouse_state.button[MouseButtons_MiddleClick] = 0;
            } break;
            case WM_RBUTTONUP:
            {
                global_input.curr_mouse_state.button[MouseButtons_RightClick] = 0;
            } break;

            case WM_LBUTTONDOWN:
            {
                global_input.curr_mouse_state.button[MouseButtons_LeftClick] = 1;

            } break;
            case WM_MBUTTONDOWN:
            {
                global_input.curr_mouse_state.button[MouseButtons_MiddleClick] = 1;
            } break;
            case WM_RBUTTONDOWN:
            {
                global_input.curr_mouse_state.button[MouseButtons_RightClick] = 1;
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

void debug_draw_menu(DebugState *debug_state);
int main()
{
    Arena arena;
    arena_init(&arena, 1024 * 1024 * 2);

    u32 window_width = 1280;
    u32 window_height = 720;
    global_w32_window =  os_win32_open_window("UI example", window_width, window_height, win32_main_callback, 0);
    global_pixel_buffer = os_win32_create_buffer(window_width, window_height);

    // fonts

    font_init();
    FontInfo font_info = font_load(&arena);
    

    // debug info
    // How does he creates and assigns the DebugState's arena from the first one???
    DebugState *debug_state = arena_push_size(&arena, DebugState, 1);
    arena_init(&debug_state->arena, 1024 * 1024 * 1);
    DebugContext debug_context; 
    debug_context.state = debug_state;

    debug_state->font_info = font_info;
    debug_state->tab_size = 4;



    Mat4x4 rot_mat = 
    Mat4x4(
        1, 5, 9,  13,
        2, 6, 10, 14,
        3, 7, 11, 15,
        4, 8, 12, 16
    );

    debug_begin_hierarchy(&debug_context, 200, 300);
    {
        debug_begin_group(&debug_context, str8("Group 1"));
        {
            debug_add_var(&debug_context, str8("elem 1"), DebugVariableType_u32);
            debug_add_var(&debug_context, str8("elem 2"), DebugVariableType_Str8);
            DebugVariable *debug_var_rot_mat = debug_add_var(&debug_context, str8("Rotation matrix"), DebugVariableType_Mat4x4);
            debug_var_rot_mat->mat4x4_value = rot_mat;
        }
        debug_end_group(&debug_context);

        debug_begin_group(&debug_context, str8("Group 2"));
        {
            debug_add_var(&debug_context, str8("elem 1"), DebugVariableType_b32);
            DebugVariable *var = debug_begin_group(&debug_context, str8("Group 3"));
            var->group.expanded = false;
            {
                debug_add_var(&debug_context, str8("elem 1"), DebugVariableType_b32);
            }
            debug_end_group(&debug_context);
        }
        debug_end_group(&debug_context);

        debug_add_var(&debug_context, str8("groupless item"), DebugVariableType_b32);


        DebugVariable *btn1;
        DebugVariable *btn2;
        DebugVariable *btn3;
        
        DebugVariable *btn_grp1 = debug_begin_group(&debug_context, str8("Group 1"));
        btn_grp1->group.expanded = false;
        {
            btn1 = debug_add_var(&debug_context, str8("btn 1"), DebugVariableType_Button);
            btn2 = debug_add_var(&debug_context, str8("btn 2"), DebugVariableType_Button);
            btn3 = debug_add_var(&debug_context, str8("btn 3"), DebugVariableType_Button);
        }
        debug_end_group(&debug_context);
    }

    #if 1
    debug_end_hierarchy(&debug_context);

    debug_begin_hierarchy(&debug_context, 500, 300);
    {
        debug_begin_group(&debug_context, str8("Group 1"));
        {
            debug_add_var(&debug_context, str8("elem 1"), DebugVariableType_b32);
            debug_add_var(&debug_context, str8("elem 2"), DebugVariableType_b32);
            DebugVariable *debug_var_rot_mat = debug_add_var(&debug_context, str8("Rotation matrix"), DebugVariableType_Mat4x4);
            debug_var_rot_mat->mat4x4_value = rot_mat;
        }
        debug_end_group(&debug_context);

    }
    debug_end_hierarchy(&debug_context);

    debug_begin_hierarchy(&debug_context, 500, 100);
    {
        debug_begin_group(&debug_context, str8("Santiago Caci"));
        {
            debug_add_var(&debug_context, str8("Edad"), DebugVariableType_u32);
            debug_add_var(&debug_context, str8("Mail"), DebugVariableType_Str8);
        }
        debug_end_group(&debug_context);

    }
    debug_end_hierarchy(&debug_context);

    #endif

    while (global_w32_window.is_running)
    {
        win32_process_pending_msgs();
        clear_buffer(&global_pixel_buffer, 0xFF000000);


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

        Point2D mouse_p;
        mouse_p.x = global_input.curr_mouse_state.x;
        mouse_p.y = global_input.curr_mouse_state.y;

        debug_state->dmouse_p.x = mouse_p.x - debug_state->mouse_p.x;
        debug_state->dmouse_p.y = mouse_p.y - debug_state->mouse_p.y;
        debug_state->mouse_p = mouse_p;

        //char buf[200];
        //sprintf(buf, "dest_x: %d, dest_y: %d\n", final_dest_x, final_dest_y);


        debug_draw_menu(debug_state);
            if(debug_state->active_interaction)
            {
                DebugHierarchyNode *hierarchy = debug_state->active_hierarchy;
                if(input_click_left_up(&global_input))
                {
                    // DEBUGEndInteract()
                    // do something like modify value or toggle group expansion
                    switch(debug_state->active_interaction)
                    {
                        case DebugInteractionType_None:
                        {

                        } break;
                        case DebugInteractionType_Toggle:
                        {
                            switch(debug_state->active_variable->type)
                            {
                                // TODO: this is fucking ugly, the logic is splitted
                                case DebugVariableType_Group:
                                {
                                    debug_state->active_variable->group.expanded = !debug_state->active_variable->group.expanded;
                                } break;
                                case DebugVariableType_b32:
                                {
                                    debug_state->active_variable->b32_value = !debug_state->active_variable->b32_value;
                                } break;
                            }
                        } break;
                        case DebugInteractionType_DragHierarchy:
                        {
                        } break;
                    }

                    debug_state->active_variable = 0;
                    debug_state->active_interaction = DebugInteractionType_None;
                }
                else
                {
                    if(input_click_left_down(&global_input))
                    {

                        if(debug_state->hot_interaction == DebugInteractionType_DragHierarchy)
                        {
                            debug_state->active_interaction = DebugInteractionType_DragHierarchy;
                            if(debug_state->active_hierarchy)
                            {
                                if(debug_state->active_interaction == DebugInteractionType_DragHierarchy)
                                {
                                    {
                                        char buf[200];
                                        char *at = buf;
                                        char *end = buf + sizeof(buf);

                                        at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "(%2.f, %2.f)", debug_state->dmouse_p.x, debug_state->dmouse_p.y);
                                        Str8 text = str8(buf, at - buf);
                                        draw_text(&global_pixel_buffer, 100, 200, text, font_info.font_table);
                                    }
                                    debug_state->active_hierarchy->AtX += debug_state->dmouse_p.x;
                                    debug_state->active_hierarchy->AtY += debug_state->dmouse_p.y;

                                }
                            }
                        }
                    }
                }
            }
            else
            {
                debug_state->hot_variable = debug_state->next_hot_variable;
                debug_state->hot_interaction = debug_state->next_hot_interaction;
                debug_state->hot_hierarchy = debug_state->next_hot_hierarchy;
                if(input_click_left_down(&global_input))
                {
                    //debug_state->active_hierarchy = debug_state->hot_hierarchy;
                    // DEBUGBeginInteract()
                    if(debug_state->hot_hierarchy && debug_state->hot_interaction == DebugInteractionType_DragHierarchy)
                    {
                        debug_state->active_interaction = DebugInteractionType_DragHierarchy;
                        debug_state->active_hierarchy = debug_state->hot_hierarchy;
                    }
                    else
                    {
                        if(debug_state->hot_variable)
                        //if(debug_state->active_hierarchy)
                        {
                            switch(debug_state->hot_variable->type)
                            {
                                // TODO: this is fucking ugly, the logic is splitted
                                case DebugVariableType_Group:
                                {
                                    debug_state->active_interaction = DebugInteractionType_Toggle;
                                } break;
                                case DebugVariableType_b32:
                                {
                                    debug_state->active_interaction = DebugInteractionType_Toggle;
                                } break;
                            }

                            if(debug_state->active_interaction)
                            {
                                debug_state->active_variable = debug_state->hot_variable;
                            }
                        }
                    }
                }
            }

        {
            // TODO make this work
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "Mouse screen coordinates (%.2f, %.2f)";
            at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, mouse_p.x, mouse_p.y);
            Str8 text = str8(buf, at - buf);
            draw_text(&global_pixel_buffer, 0, 100, text, font_info.font_table);

            /*
            
            char buf[200];
            char *at = buf;
            char *end = buf + sizeof(buf);

            at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "(%2.f, %2.f)", debug_state->dmouse_p.x, debug_state->dmouse_p.y);
            Str8 text = str8(buf, at - buf);
            draw_text(&global_pixel_buffer, 100, 200, text, font_info.font_table);
            */
        }


        //debug_state->active = 0;
        // DEBUGEnd()
        debug_state->next_hot_variable = 0;
        debug_state->next_hot_interaction = DebugInteractionType_None;
        debug_state->next_hot_hierarchy = 0;
        //debug_state->hot = debug_state->next_hot;
        //debug_state->hot_hierarchy = 0;
        //debug_state->active_hierarchy = 0;
        

        OS_Window_Dimension win_dim = os_win32_get_window_dimension(global_w32_window.handle);
        HDC device_context = GetDC(global_w32_window.handle);
        os_win32_display_buffer(device_context, &global_pixel_buffer, win_dim.width, win_dim.height);
        ReleaseDC(global_w32_window.handle, device_context);
        input_update(&global_input);
    };
}

void debug_draw_menu(DebugState *debug_state)
{
    for(DebugHierarchyNode* h_node = debug_state->root_hierarchy.first; h_node; h_node = h_node->next)
    {
        /* OBS: I could also modify the position of the hierarchy here
        like: 
        ```
        f32 AtX = h_node->AtX;
        f32 AtY = h_node->AtY;
        if (debug_state->hot_hierarchy == h_node) 
        {
            AtX = h_node->AtX;
            AtY = h_node->AtY;
        }

        ```
        */

        f32 AtX = h_node->AtX;
        f32 AtY = h_node->AtY;
        //DebugVariable *var = var->group.first_child;
        DebugVariable *var = h_node->root_var->group.first_child;
        while(var)
        {
            char buf[4096];
            char *at = buf;
            char *end = buf + sizeof(buf);
            u32 inc = 0;
            Str8 text;
            if(var->type == DebugVariableType_Group)
            {
                if (var->group.expanded)
                {
                    //inc = _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "[-] %s", var->name.str);
                    inc = _snprintf_s(buf, (size_t)(end - buf), (size_t)(end - buf), "[-] %s", var->name.str);
                    text = str8(buf, inc);
                    //text = str8_fmt("[-] %s", var->name.str);
                }
                else
                {
                    inc = _snprintf_s(buf, (size_t)(end - buf), (size_t)(end - buf), "[+] %s", var->name.str);
                    text = str8(buf, inc);

                    //inc = _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "[+] %s", var->name.str);
                }
            }
            else
            {
                switch(var->type) 
                {
                    case DebugVariableType_Mat4x4:
                    {
                        at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "%s: \n\t[", var->name.str);

                        for(u32 i = 0; i < 4; i++)
                        {
                            for(u32 j = 0; j < 4; j++)
                            {

                                if (j != 3)
                                {
                                    at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "%2.f, ", var->mat4x4_value.m[j][i]);
                                }
                                else
                                {
                                    at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "%2.f]", var->mat4x4_value.m[j][i]);

                                }

                                if (j == 3 && i != 3) 
                                {
                                    at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "\n\t[");
                                }
                            }
                        }
                        text = str8(buf, at - buf);
                    } break;
                    case DebugVariableType_b32:
                    {
                        at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "%.*s: %s", u32(var->name.size), var->name.str, var->b32_value ? "true" : "false");
                        text = str8(buf, at - buf);
                    } break;
                    default:
                    {
                        text = var->name;
                    } break;
                }
                //inc = _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "%s", var->name.str);
            }

            TextSize text_size = font_get_text_size(&debug_state->font_info, text);
            Rect2D bbox = {AtX, AtY - debug_state->font_info.ascent, text_size.w, text_size.h};
            if(rect_contains_point(bbox, debug_state->mouse_p))
            {
                debug_state->next_hot_hierarchy = h_node;
                debug_state->next_hot_variable = var;
            }

            Rect2D size_box = {h_node->AtX - 20, h_node->AtY - 30, 10, 10};
            draw_rect(&global_pixel_buffer, size_box, 0xFFFFFFFF);
            if(rect_contains_point(size_box, debug_state->mouse_p) && input_click_left_down(&global_input))
            {
                debug_state->next_hot_interaction = DebugInteractionType_DragHierarchy;
                debug_state->next_hot_hierarchy = h_node;
                // NOTE this is wrong! replace by a debughierarchy to be able to move them!
                //debug_state->next_hot_variable = var;
            }


            u32 text_color = 0xFFFFFFFF;
            if(var == debug_state->hot_variable)
            {
                text_color = 0xFF00FFFF;

                // draw bounding box selection
                #if 0
                draw_line(&global_pixel_buffer, bbox.x, bbox.y , bbox.x + bbox.w, bbox.y, 0xFFFFFF00);
                draw_line(&global_pixel_buffer, bbox.x, bbox.y + bbox.h, bbox.x + bbox.w, bbox.y + bbox.h, 0xFFFFFF00);

                draw_line(&global_pixel_buffer, bbox.x, bbox.y, bbox.x, bbox.y + bbox.h, 0xFFFFFF00);
                draw_line(&global_pixel_buffer, bbox.x + bbox.w, bbox.y , bbox.x + bbox.w, bbox.y + bbox.h, 0xFFFFFF00);
                #endif
            }
            draw_text(&global_pixel_buffer, bbox.x, bbox.y + debug_state->font_info.ascent, text, debug_state->font_info.font_table, text_color);
            //if(str8_equals(var->name, str8("Group 2"))){

            //}

            //debug_state->AtY += debug_state->font_info.ascent + debug_state->font_info.descent + 20;
            AtY += (debug_state->font_info.ascent + debug_state->font_info.descent) * text_size.lines + 10;
            if (var->type == DebugVariableType_Group && var->group.expanded)
            {
                AtX +=  debug_state->tab_size * debug_state->font_info.max_char_width >> 6;
                if(var->group.first_child)
                {
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
                        AtX -= debug_state->tab_size * debug_state->font_info.max_char_width >> 6;
                    }
                }
            }
        }
    }
}