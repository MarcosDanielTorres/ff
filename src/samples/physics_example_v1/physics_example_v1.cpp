/*
    How do I change the acceleration of a particle? Forces F = ma => a = F/m
        The more mass the harder to accelerate
    Weight = g * m which then cancels out when applying a = F/m, so no need to add it to F, just to accel after a = F/m
    Drag: works in the opposite direction of the motion of the object with respect to a surrounding fluid (water, oil, mud, air, gas)
        Depends on the velocity, the more velocity the stronger the drag

    Drag = 0.5 * fluid_density (p) * coefficient of drag (k) * cross-sectional area (A) * length_squared(velocity) * -norm(velocity)
    p, k and A are just constants
*/
#include "base/base_core.h"
#include "base/base_arena.h"
#include "base/base_string.h"
#include "os/os_core.h"
#include "draw/draw.h"
#include "font/font.h"
#include "base/base_math.h"

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

struct Particle
{
    Vec2 pos;
    Vec2 vel;
    Vec2 acc;
    f32 inv_m;
    f32 r;
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

struct State
{
    FontInfo font_info;
    Point2Dx dmousep;
};


b32 rect_contains_point(Rect2Dx rect, Point2Dx point) {
    b32 result = ((point.x > rect.x) &&
                  (point.x < rect.x + rect.w) && 
                  (point.y < rect.y) &&
                  (point.y > rect.y - rect.h));
    return result;
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
    }

    PlatformLimits platform_limits = {};
    platform_limits.max_quad_count_per_frame = (1 << 20);
    // TODO(Marcos): Should I do some VirtualAlloc?!
    OpenGL* opengl = arena_push_size(&g_arena, OpenGL, 1);
    opengl_init(opengl, g_w32_window, &platform_limits, &state->font_info);

    LONGLONG frequency = aim_timer_get_os_freq();
    LONGLONG last_frame = 0;
    g_w32_window.is_running = true;
    glViewport(0, 0, SRC_WIDTH, SRC_HEIGHT);

    HDC hdc = GetDC(g_w32_window.handle);

    glm::mat4 ortho_proj_mat = glm::ortho(0.0f, f32(SRC_WIDTH), f32(SRC_HEIGHT), 0.0f, -1.0f, 1.0f);


    Point2Dx prev_frame_mouse_p = {};
    u32 frame_window = 100;
    u32 frame_counter = 0;
    f32 avg_frame_time = 0;
    f32 total_frame_time_per_frame_window = 0;
    LONGLONG now = aim_timer_get_os_time();
    LONGLONG dt_long = now - last_frame;
    last_frame = now;

    f32 pixels_per_meter = 50.f;
    Particle *particles = arena_push_size(&g_arena, Particle, 10);
    u32 count = 3;

    particles[0] = {.pos = Vec2{100.0f, 100.0f}, .vel = Vec2{}, .acc = Vec2{}, .inv_m = 1.0f/3.0f, .r = 10.0f};
    particles[1] = {.pos = Vec2{200.0f, 100.0f}, .vel = Vec2{}, .acc = Vec2{}, .inv_m = 1.0f/4.0f, .r = 3.0f};
    particles[2] = {.pos = Vec2{300.0f, 800.0f}, .vel = Vec2{}, .acc = Vec2{}, .inv_m = 1.0f, .r = 3.0f};

    while (g_w32_window.is_running)
	{
        win32_process_pending_msgs();
        if(frame_counter == frame_window)
        {
            avg_frame_time = total_frame_time_per_frame_window / frame_counter;
            frame_counter = 0;
            total_frame_time_per_frame_window = 0;
        }

        while ((aim_timer_ticks_to_ms(aim_timer_get_os_time() - last_frame, frequency)) < (1000.0f/60.0f)) {}

        now = aim_timer_get_os_time();
        dt_long = now - last_frame;
        f32 dt = aim_timer_ticks_to_sec(dt_long, frequency);
        dt = Min(dt, 0.016f);
        f32 dt_ms = aim_timer_ticks_to_ms(dt_long, frequency);
        last_frame = now;
        total_frame_time_per_frame_window += dt_ms;

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
            push_text(render_group, &state->font_info, at, 15, 30);
        }

        {
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "mouse pos: %.2f, %.2f";
            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, mouse_p.x, mouse_p.y);
            push_text(render_group, &state->font_info, at, 15, 50);
        }

        {
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "avg frame time: %.3fms";
            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, avg_frame_time);
            push_text(render_group, &state->font_info, at, 15, 70);

        }

        {
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "frame number: %d";
            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, frame_counter);
            push_text(render_group, &state->font_info, at, 270, 70);

        }


        {
            {
            #if 0
                char buf[100];
                char *at = buf;
                char *end = buf + sizeof(buf);
                const char* c  = "vel: (%f, %f) m/s";
                _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, p.vel.x, p.vel.y);
                push_text(render_group, &state->font_info, at, 15, 90);
            #endif
            }

            Vec2 g = Vec2{0.0f, pixels_per_meter * 9.8f};
            Vec2 f = {};
            f += Vec2{pixels_per_meter * 0.2f, 0.0f};
            //f += Vec2{0.0f, pixels_per_meter * 1.0f};

            for(u32 i = 0; i < count; i++)
            {
                Particle *p = particles + i;
                p->acc = f * p->inv_m;
                p->acc += g;
                p->vel += p->acc * dt;
                p->pos += p->vel * dt;

                if (p->pos.x + p->r > SRC_WIDTH)
                {
                    p->pos.x = SRC_WIDTH - p->r;
                    p->vel.x *= -1.0f;
                }
                if (p->pos.x - p->r < 0)
                {
                    p->pos.x = p->r;
                    p->vel.x *= -1.0f;
                }
                if (p->pos.y + p->r > SRC_HEIGHT)
                {
                    p->pos.y = SRC_HEIGHT - p->r;
                    p->vel.y *= -1.0f;
                }
                if (p->pos.y - p->r < 0)
                {
                    p->pos.y = p->r;
                    p->vel.y *= -1.0f;
                }


                push_circle(render_group, p->pos.x, p->pos.y, p->r);

            }

        }

        {
            opengl->glUseProgram(opengl->program_id);
            opengl->glUniformMatrix4fv(opengl->proj, 1, GL_FALSE, &ortho_proj_mat[0][0]);
            opengl->glUniform1i(opengl->texture_sampler, 0);
            render_frame(opengl, render_group);
        }

        //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);

        SwapBuffers(hdc);
        temp_end(per_frame);
        frame_counter++;
        input_update(&g_input);
    }
    ReleaseDC(g_w32_window.handle, hdc);
    aim_profiler_print();
    aim_profiler_end();
}