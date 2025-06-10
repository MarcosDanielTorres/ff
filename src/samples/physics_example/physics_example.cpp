#include "base/base_core.h"
#include "base/base_arena.h"
#include "base/base_string.h"
#include "os/os_core.h"
#include "draw/draw.h"
#include "font/font.h"

#include "base/base_arena.cpp"
#include "base/base_string.cpp"
#include "os/os_core.cpp"
#include "font/font.cpp"
#include "input/input.h"

typedef Input GameInput;

#include <math.h>
// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>


#define AIM_PROFILER
#include "aim_timer.h"
#include "aim_profiler.h"

#include "aim_timer.cpp"
#include "aim_profiler.cpp"

// globals
global_variable OS_Window g_w32_window;
global_variable Arena g_arena;
global_variable Arena g_transient_arena;
global_variable u32 SRC_WIDTH = 1680;
global_variable u32 SRC_HEIGHT = 945;

global_variable u32 os_modifiers;
global_variable GameInput g_input;

#include "bindings/opengl_bindings.cpp"
#include "renderer/renderer.h"
#include "renderer/renderer.cpp"

struct PlatformLimits
{
    u32 max_quad_count_per_frame;
    u32 max_index_count;
};


#include "renderer/opengl_renderer.cpp"

void win32_process_pending_msgs() 
{
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&Message);
        switch(Message.message)
        {
            case WM_QUIT:
            {
                g_w32_window.is_running = false;
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
                g_input.curr_keyboard_state.keys[key] = u8(is_pressed);
                g_input.prev_keyboard_state.keys[key] = u8(was_pressed);
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
                if(xPos >= 0 && xPos < SRC_WIDTH && yPos >= 0 && yPos < SRC_HEIGHT)
                {
                }
                // comente esto
                //SetCapture(g_w32_window.handle);

                i32 xxPos = LOWORD(Message.lParam);
                i32 yyPos = HIWORD(Message.lParam);
                char buf[100];
                sprintf(buf,  "MOUSE MOVE: x: %d, y: %d\n", xPos, yPos);
                //printf(buf);

                Assert((xxPos == xPos && yyPos == yPos));
                g_input.curr_mouse_state.x = xPos;
                g_input.curr_mouse_state.y = yPos;
                
                g_input.dx = g_input.curr_mouse_state.x - g_input.prev_mouse_state.x;
                g_input.dy = -(g_input.curr_mouse_state.y - g_input.prev_mouse_state.y);

            }
            break;
            case WM_LBUTTONUP:
            {
                g_input.curr_mouse_state.button[MouseButtons_LeftClick] = 0;
                g_input.prev_mouse_state.button[MouseButtons_LeftClick] = 1;

            } break;
            case WM_MBUTTONUP:
            {
                g_input.curr_mouse_state.button[MouseButtons_MiddleClick] = 0;
            } break;
            case WM_RBUTTONUP:
            {
                g_input.curr_mouse_state.button[MouseButtons_RightClick] = 0;
            } break;

            case WM_LBUTTONDOWN:
            {
                g_input.curr_mouse_state.button[MouseButtons_LeftClick] = 1;
                g_input.prev_mouse_state.button[MouseButtons_LeftClick] = 0;

            } break;
            case WM_MBUTTONDOWN:
            {
                g_input.curr_mouse_state.button[MouseButtons_MiddleClick] = 1;
            } break;
            case WM_RBUTTONDOWN:
            {
                g_input.curr_mouse_state.button[MouseButtons_RightClick] = 1;
            } break;
            default:
            {
                DispatchMessageA(&Message);
            } break;
        }
    }
}

LRESULT CALLBACK win32_main_callback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) 
{
    LRESULT result = 0;
    switch(Message)
    {
        case WM_DESTROY:
        {
            g_w32_window.is_running = false;
        } break;
        case WM_SIZE:
        {
            RECT r;
            GetClientRect(g_w32_window.handle, &r);
            u32 width = r.right - r.left;
            u32 height = r.bottom - r.top;
            if (width != SRC_WIDTH || height != SRC_HEIGHT)
            {
                glViewport(0, 0, width, height);
            }
        }break;
        default:
        {
            result = DefWindowProcA(Window, Message, wParam, lParam);
        } break;
    }
    return result;
}

enum UI_Interaction
{
    Interaction_None,
    Interaction_Drag,
};

struct Widget
{
    f32 x, y, w, h;
    Str8 text; 
    glm::vec4 c;
    Widget *next;
};

struct Rect2Dx
{
    f32 x;
    f32 y;
    f32 w;
    f32 h;
};

struct Point2Dx
{
    f32 x;
    f32 y;
};

Point2Dx operator-(Point2Dx a, Point2Dx b)
{
    Point2Dx result = { a.x - b.x, a.y - b.y};
    return result;
}

Point2Dx operator+(Point2Dx a, Point2Dx b)
{
    Point2Dx result = { a.x + b.x, a.y + b.y};
    return result;
}

Point2Dx operator*(Point2Dx a, f32 s)
{
    Point2Dx result = { a.x * s, a.y * s};
    return result;
}

Point2Dx &operator+=(Point2Dx &a, Point2Dx b)
{
    a = { a.x + b.x, a.y + b.y};
    return a;
}

struct WidgetList
{
    Widget *first;
    Widget *last;
};

struct State
{
    FontInfo font_info;
    WidgetList *w_list;

    UI_Interaction interaction;
    UI_Interaction next_interaction;
    Widget *active;
    Widget *hot;
    Widget *next_hot;
    Point2Dx dmousep;
};


b32 rect_contains_point(Rect2Dx rect, Point2Dx point) {
    b32 result = ((point.x > rect.x) &&
                  (point.x < rect.x + rect.w) && 
                  (point.y < rect.y) &&
                  (point.y > rect.y - rect.h));
    return result;
}

internal void
add_widget(Arena *arena, WidgetList* w_list, Widget w)
{
    if (w_list->first == w_list->last)
    {
        Widget *new_w = (Widget *)arena_push_size(arena, Widget, 1);
        *new_w = w;
        w_list->last = new_w;
        w_list->first->next = w_list->last;
    }
    else
    {
        Widget *new_w = (Widget *)arena_push_size(arena, Widget, 1);
        *new_w = w;
        w_list->last->next = new_w;
        w_list->last = new_w;
    }
}

int main()
{
    aim_profiler_begin();
    g_w32_window = os_win32_open_window("opengl", SRC_WIDTH, SRC_HEIGHT, win32_main_callback, 1);
	
	arena_init(&g_arena, mb(20));
	arena_init(&g_transient_arena, mb(300));


    State *state = arena_push_size(&g_arena, State, 1);
    {
        // fonts
        font_init();
        FontInfo font_info = font_load(&g_arena);
        state->font_info = font_info;
        state->w_list = arena_push_size(&g_arena, WidgetList, 1);
    }

    PlatformLimits platform_limits = {};
    platform_limits.max_quad_count_per_frame = (1 << 20);
    // TODO(Marcos): Should I do some VirtualAlloc?!
    OpenGL* opengl = arena_push_size(&g_arena, OpenGL, 1);
    opengl_init(opengl, g_w32_window, &platform_limits, &state->font_info);

    // ui state init
    LONGLONG frequency = aim_timer_get_os_freq();
    LONGLONG last_frame = 0;
    Point2Dx prev_frame_mouse_p = {};
    b32 click_left_pressed_last_frame = 0;
    g_w32_window.is_running = true;
    glViewport(0, 0, SRC_WIDTH, SRC_HEIGHT);

    HDC hdc = GetDC(g_w32_window.handle);

    glm::mat4 ortho_proj_mat = glm::ortho(0.0f, f32(SRC_WIDTH), f32(SRC_HEIGHT), 0.0f, -1.0f, 1.0f);


    Widget window1 {};
    window1.x = 800.0f;
    window1.y = 500.0f;
    window1.w = 200.0f;
    window1.h = 100.0f;
    window1.c = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

    //push_circle(render_group, glm::vec3(500.0f, 500.0f, 0.0f), 20);
    //Widget circle {};
    //circle.x = 500.0f;
    //circle.y = 500.0f;
    //circle.w = 20.0f;
    //circle.c = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    Widget some_rect {};
    circle.x = 500.0f;
    circle.y = 500.0f;
    circle.w = 20.0f;
    circle.h = 200.0f;
    circle.c = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    //push_rect(render_group, 800.0f, 800.0f, 100.0f, 100.0f, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    Widget border {};
    border.x = 800.0f;
    border.y = 800.0f;
    border.w = 100.0f;
    border.h = 100.0f;
    border.c = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

    //add_widget(&g_arena, state->w_list, circle);
    add_widget(&g_arena, state->w_list, window1);
    add_widget(&g_arena, state->w_list, border);
    add_widget(&g_arena, state->w_list, some_rect);

    while (g_w32_window.is_running)
	{
        LONGLONG now = aim_timer_get_os_time();
        LONGLONG dt_long = now - last_frame;
        last_frame = now;
        f32 dt = aim_timer_ticks_to_sec(dt_long, frequency);
        f32 dt_ms = aim_timer_ticks_to_ms(dt_long, frequency);

        Point2Dx mouse_p = {g_input.curr_mouse_state.x, g_input.curr_mouse_state.y};
        state->dmousep = mouse_p - prev_frame_mouse_p;

        prev_frame_mouse_p = mouse_p;

        TempArena per_frame = temp_begin(&g_transient_arena);

        // begin render group
        RenderGroup *render_group = arena_push_size(per_frame.arena, RenderGroup, 1);
        render_group->vertex_array = arena_push_size(per_frame.arena, TextureQuadVertex, opengl->max_vertex_count);
        render_group->index_array = arena_push_size(per_frame.arena, u16, opengl->max_index_count);
        render_group->vertex_count = 0;
        render_group->index_count  = 0;
        render_group->max_vertex_count = opengl->max_vertex_count;
        render_group->max_index_count  = opengl->max_index_count;

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


        g_input.dt = dt;
        {
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "Frame time: %.4fms";
            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, dt_ms);
            push_text(render_group, &state->font_info, at, 25, 75);
        }

        {
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "mouse pos: %.2f, %.2f";
            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, mouse_p.x, mouse_p.y);
            push_text(render_group, &state->font_info, at, 25, 100);
        }
        {
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "delta mouse pos: %.2f, %.2f";
            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, state->dmousep.x, state->dmousep.y);
            push_text(render_group, &state->font_info, at, 25, 125);
        }

        if(input_was_click_left_down(&g_input))
        {
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "was pressed";
            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c);
            push_text(render_group, &state->font_info, at, 25, 155);

        }


        b32 hovered = 0;
        for(Widget *w = state->w_list->first; w != 0; w = w->next)
        {
            Rect2Dx dim = {w->x, w->y, w->w, w->h};
            //f32 r = state->circle.w;
            //Rect2Dx circle_rect = {state->circle.x - r, state->circle.y + r, r * 2, r * 2};
            //Rect2Dx border_rect = {state->border.x, state->border.y, state->border.w, state->border.h};

            if(hovered == 0 && rect_contains_point(dim, mouse_p))
            {
                state->next_hot = w;
                hovered = 1;
            }

            {
                f32 x = w->x;
                f32 y = w->y;
                f32 width = w->w;
                f32 height = w->h;
                glm::vec4 c = w->c;
                if (w == state->hot)
                {

                    push_rect(render_group, x, y, width, height, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
                }
                else
                {
                    push_rect(render_group, x, y, width, height, c);
                }
            }

        }


        /*
            Point2Dx dtm = state->dmousep;
            state->active->x += dtm.x;
            state->active->y += dtm.y;
        */

        //AssertGui(g_input.prev_mouse_state.button[MouseButtons_LeftClick] == g_input.curr_mouse_state.button[MouseButtons_LeftClick], "Finally!");

        if (input_click_left_down(&g_input))
        {
            if (state->hot)
            {
                Rect2Dx rect = {state->hot->x,state->hot->y,state->hot->w,state->hot->h};
                if(rect_contains_point(rect,  mouse_p))
                {
                    state->active = state->hot;
                }
            }
            else
            {
                state->active = 0;
            }
            if (state->active)
            {
                Point2Dx dtm = state->dmousep;
                state->active->x += dtm.x;
                state->active->y += dtm.y;
                state->active = state->hot;
            }
        }


        if (state->active)
        {
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "Active: %p";
            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, state->active);
            push_text(render_group, &state->font_info, at, 25, 145);
        }

        //printf("curr: %d, prev: %d\n", g_input.curr_mouse_state.button[MouseButtons_LeftClick], g_input.prev_mouse_state.button[MouseButtons_LeftClick]);

        {
            //glm::vec3 from = glm::vec3(0.0f, 0.0f, 0.0f);
            //glm::vec3 to = glm::vec3(10.0f, 2.0f, 10.0f);
            //push_line(render_group, from, to);
        }


        win32_process_pending_msgs();

        //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);

        {
            opengl->glUseProgram(opengl->program_id);
            opengl->glUniformMatrix4fv(opengl->proj, 1, GL_FALSE, &ortho_proj_mat[0][0]);
            opengl->glUniform1i(opengl->texture_sampler, 0);
            render_frame(opengl, render_group);
        }

        // reset ui state!

        state->hot = state->next_hot;
        state->next_hot = 0;
        if (!state->active)
            state->interaction = Interaction_None;
        //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);

        click_left_pressed_last_frame = input_click_left_down(&g_input);
        input_update(&g_input);
        SwapBuffers(hdc);
        temp_end(per_frame);
    }
    ReleaseDC(g_w32_window.handle, hdc);
    aim_profiler_print();
    aim_profiler_end();
}