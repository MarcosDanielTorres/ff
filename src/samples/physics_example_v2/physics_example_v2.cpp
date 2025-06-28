/*
    How do I change the acceleration of a particle? Forces F = ma => a = F/m
        The more mass the harder to accelerate
    Weight = g * m which then cancels out when applying a = F/m, so no need to add it to F, just to accel after a = F/m
    Drag: works in the opposite direction of the motion of the object with respect to a surrounding fluid (water, oil, mud, air, gas)
        Depends on the velocity, the more velocity the stronger the drag

    Drag = 0.5 * fluid_density (p) * coefficient of drag (k) * cross-sectional area (A) * length_squared(velocity) * -norm(velocity)
    p, k and A are just constants

    Springs = Hooke's law: F = -kx = =
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

global_variable f32 pixels_per_meter = 50.f;

global_variable f32 total_time = 0.0f;

#include "bindings/opengl_bindings.cpp"
#include "renderer/renderer.h"
#include "renderer/renderer.cpp"

struct PlatformLimits
{
    u32 max_quad_count_per_frame;
    u32 max_index_count;
};

// #p
struct Particle
{
    Vec2 pos;
    Vec2 vel;
    Vec2 acc;
    Vec2 f;
    f32 inv_m;
    f32 r;
};

typedef u32 ShapeFlags;
enum
{
    ShapeFlag_None = (1 << 0),
    ShapeFlag_Circle = (1 << 1),
    ShapeFlag_Box = (1 << 2),
};

enum ShapeType
{
    ShapeType_Circle,
    ShapeType_Box,
    ShapeType_Count,
};

struct Entity
{
    Vec2 p;
    Vec2 v;
    Vec2 a;
    f32 inv_m;
    f32 ap;
    f32 av;
    f32 aa;
    f32 inv_i;

    Vec2 t_f;
    f32 t_t;

    ShapeFlags flags;
    f32 r, w, h;

    ShapeType type;
    union
    {
        struct
        {
            f32 r;
        };
        struct
        {
            f32 w, h;
        };
    };
};

internal f32
inertia_circle(f32 inv_m, f32 r)
{
    // 0.5f*m*r*r <=> (2.0f * 1)/(m*r*r) <=> (2.0f*inv_m)/(r*r)
    f32 result = (2.0f * inv_m) / (r * r);
    return result;
}


Vec2 spring_force(Vec2 anchor, Vec2 p, f32 eq_len, f32 k)
{
    Vec2 d = p - anchor;
    f32 disp = len(d) - eq_len;
    d = norm(d);
    return disp * d * -k;
}

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

void process_physics(Particle *particles, u32 count, float dt)
{
    Vec2 g = Vec2{0.0f, pixels_per_meter * 9.8f};
    Vec2 f = {};
    f += Vec2{pixels_per_meter * 0.2f, 0.0f};

    for(u32 i = 0; i < count; i++)
    {
        Particle *p = particles + i;
        p->acc = f * p->inv_m;
        p->acc += g;
        p->vel += p->acc * dt;
        p->pos += p->vel * dt;

        // collisions
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

    u32 entity_count = 2;
    Entity *entity = arena_push_size(&g_arena, Entity, 5);
    entity[0] = {.p = Vec2{900.0f, 400.0f}, .inv_m = 1.0f, .r = 40.0f};
    entity[0].inv_i = inertia_circle(entity->inv_m, entity->r);
    entity[0].type = ShapeType_Circle;

    entity[1] = {.p = Vec2{300.0f, 400.0f}, .inv_m = 1.0f, .w = 300.0f, .h = 90.0f};
    entity[1].inv_i = inertia_circle(entity->inv_m, entity->r);
    entity[1].type = ShapeType_Box;

    Particle *particles = arena_push_size(&g_arena, Particle, 10);
    particles[0] = {.pos = Vec2{700.0f, 400.0f}, .vel = Vec2{}, .acc = Vec2{}, .inv_m = 1.0f, .r = 40.0f};
    u32 count = 1;

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
        total_time += dt;

        local_persist f32 angle = 0.0f;
        // debug text
        push_text(render_group, &state->font_info, str8_fmt(per_frame.arena, "Frame time: %.4fms", dt_ms), 15, 30);
        push_text(render_group, &state->font_info, str8_fmt(per_frame.arena, "mouse pos: %.2f, %.2f", mouse_p.x, mouse_p.y), 15, 50);
        push_text(render_group, &state->font_info, str8_fmt(per_frame.arena, "avg frame time: %.3fms", avg_frame_time), 15, 70);
        push_text(render_group, &state->font_info, str8_fmt(per_frame.arena, "theta: %.2frads", -1.0f * entity->ap), 15, 90);
        // debug text

        //process_physics(particles, count, dt);
        for(u32 i = 0; i < entity_count; i++)
        {
            Entity *e = entity + i;
            e->t_f = {5.0f, 0.0f};
            e->t_t = angle;
        }
        for(u32 i = 0; i < entity_count; i++)
        {
            Entity *e = entity + i;
            e->a = e->t_f * e->inv_m;
            e->v += e->a * dt;
            e->p += e->v * dt;
            e->aa = e->t_t * e->inv_i;
            e->av += e->aa * dt;
            e->ap += e->av * dt;
        }
        for(u32 i = 0; i < entity_count; i++)
        {
            Entity *e = entity + i;

            switch(e->type)
            {
                case ShapeType_Circle:
                {
                    push_circle(render_group, e->p, e->r, 0.0f, glm::vec4(1.0, 1.0, 0.0, 1.0));
                    push_line_2d(render_group, e->p, e->p + Vec2{cos(e->ap) * e->r, sin(e->ap) * e->r}, 1.0f, glm::vec4(1.0, 1.0, 1.0, 1.0));
                    // draw cm
                    push_circle(render_group, e->p, 2.0f, glm::vec4(1.0, 0.0, 0.0, 1.0));
                } break;
                case ShapeType_Box:
                {
                    f32 x, y, w, h;
                    x = e->p.x;
                    y = e->p.y;
                    w = e->w;
                    h = e->h;
                    Vec2 center = e->p + Vec2{e->w / 2.0f, -e->h / 2.0f};
                    f32 c = cos(e->ap);
                    f32 s = sin(e->ap);
                    Vec2 local[4] = {
                        { -e->w*0.5f,  +e->h*0.5f },   // bottom-left
                        { +e->w*0.5f,  +e->h*0.5f },   // bottom-right
                        { +e->w*0.5f,  -e->h*0.5f },   // top-right
                        { -e->w*0.5f,  -e->h*0.5f },   // top-left
                    };
                    Vec2 quad_points[4] = 
                    {
                        center + Vec2{local[0].x * c - local[0].y * s, local[0].x * s + local[0].y * c}, 
                        center + Vec2{local[1].x * c - local[1].y * s, local[1].x * s + local[1].y * c},
                        center + Vec2{local[2].x * c - local[2].y * s, local[2].x * s + local[2].y * c},
                        center + Vec2{local[3].x * c - local[3].y * s, local[3].x * s + local[3].y * c}, 
                    };
                    /* TODO(Marcos): Fix this one day (probably never)!
                        Quad has only the outline drawn if: border_radius = 0.03f (or a small value other than 0.0f)
                        and border_thickness = 0.0f
                        Would be better if i could just say: only outline, or not, border or not, border and outline yes, border alone yes, outline alone yes
                        Which i can but is not that clear

                        When trying I removed the if (r > 0.0) in the shader. Although it was just a hunch
                    */
                    push_rect(render_group, quad_points, 0.0f, 0.0f, 1.0f, glm::vec4(1.0f, 1.0f, 0.0f, 0.0f));

                    push_circle(render_group, quad_points[0], 2.0f, glm::vec4(0.0, 1.0, 1.0, 1.0));
                    push_circle(render_group, quad_points[1], 2.0f, glm::vec4(0.0, 1.0, 1.0, 1.0));
                    push_circle(render_group, quad_points[2], 2.0f, glm::vec4(0.0, 1.0, 1.0, 1.0));
                    push_circle(render_group, quad_points[3], 2.0f, glm::vec4(0.0, 1.0, 1.0, 1.0));
                        
                    // draw cm
                    push_circle(render_group, center, 22.0f, glm::vec4(1.0, 0.0, 0.0, 1.0));
                } break;
            };
        }
        angle -= dt * 10.4;

        {
            opengl->glUseProgram(opengl->program_id);
            opengl->glUniformMatrix4fv(opengl->proj, 1, GL_FALSE, &ortho_proj_mat[0][0]);
            opengl->glUniform1i(opengl->texture_sampler, 0);
            render_frame(opengl, render_group);
        }

        //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);

        if(input_is_key_pressed(&g_input, Keys_Q))
        {
            g_w32_window.is_running = false;
        }

        SwapBuffers(hdc);
        temp_end(per_frame);
        frame_counter++;
        input_update(&g_input);
    }
    ReleaseDC(g_w32_window.handle, hdc);
    aim_profiler_print();
    aim_profiler_end();
}