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
#define PI 3.14

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

global_variable Vec2 spr_p = {800.0f, 25.0f};
global_variable f32 spr_h_w = 40;
global_variable f32 spr_h_h = 20;
global_variable Vec2 spr_start = {spr_p.x + spr_h_w, spr_p.y};
global_variable Vec2 spr_end = Vec2{};
global_variable Vec2 spr_eq = Vec2{spr_p.x + spr_h_w, spr_p.y};
global_variable Vec2 spr_disp = {};
global_variable b32 apply_spr = 0;
global_variable b32 spring_activated = 0;

global_variable Vec2 ch_p_f = {};
global_variable f32 spr_rest_length = 200.0f;
global_variable f32 total_time = 0.0f;
global_variable Vec2 push_force;

float A1 = 40.0f, f1 = 0.1f;
float A2 = 20.0f, f2 = 0.27f;
float A3 = 10.0f, f3 = 0.62f;

float windState = 0.0f;
float theta     = 1.0f;     // how quickly it “forgets” past
float sigma     = 30.0f;    // strength of random kicks


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
    Vec2 f;
    f32 inv_m;
    f32 r;
};

struct System
{
    Vec2 p[16];
    Vec2 v[16];
    Vec2 a[16];
    f32 inv_m[16];
    f32 r[16];
    f32 t_f[16];
    f32 k;
};

struct SpringBox
{
    Particle *p;
    u32 adj_list[4][3];
    f32 k;
    u32 pc;
    Vec2 cm;
    f32 t_inv_m;
};


Vec2 spring_force(Vec2 anchor, Vec2 p, f32 eq_len, f32 k)
{
    Vec2 d = p - anchor;
    f32 disp = len(d) - eq_len;
    d = norm(d);
    return disp * d * -k;
}
float RandomNormal(void) {
    static int   hasSpare = 0;
    static float spare;

    if (hasSpare) {
        hasSpare = 0;
        return spare;
    }

    hasSpare = 1;
    float u, v, s;
    do {
        u = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        v = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        s = u*u + v*v;
    } while (s >= 1.0f || s == 0.0f);

    float mul   = sqrtf(-2.0f * logf(s) / s);
    spare       = v * mul;
    return u * mul;
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

    for(u32 i = 0; i < count - 1; i++)
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
    srand(1321);
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

    Particle *particles = arena_push_size(&g_arena, Particle, 10);
    System *system = arena_push_size(&g_arena, System, 1);
    SpringBox *spring_box = arena_push_size(&g_arena, SpringBox, 1);
    spring_box->pc = 4;
    spring_box->p = arena_push_size(&g_arena, Particle, spring_box->pc);
    u32 count = 3;

    particles[0] = {.pos = Vec2{300.0f, 800.0f}, .vel = Vec2{}, .acc = Vec2{}, .inv_m = 1.0f, .r = 3.0f};
    particles[1] = {.pos = Vec2{200.0f, 100.0f}, .vel = Vec2{}, .acc = Vec2{}, .inv_m = 1.0f/4.0f, .r = 3.0f};
    Vec2 gg = {spr_start.x, spr_start.y + 150};
    particles[2] = {.pos = gg, .vel = Vec2{}, .acc = Vec2{}, .inv_m = 1.0f/3.0f, .r = 10.0f};

    {
        // init system
        system->k = 80.0f;
        system->p[0] = {500.0f, 270.0f};
        system->inv_m[0] = 1.0f / 10.0f;
        system->r[0] = 10.0f;
        f32 spacing = 100.0f;
        f32 dx = spacing;
        f32 dy = 0.0f;
        for (u32 i = 1; i < 16; i++)
        {
            if(i % 4 == 0)
            {
                dy += spacing;
                dx = -3.0f * spacing;
            }
            system->p[i] = system->p[i - 1] + Vec2{dx, dy};
            dy = 0.0f;
            dx = spacing;
            system->inv_m[i] = 1.0f / 10.0f;
            system->r[i] = 10.0f;
        }
    }

    {
        // init spring box

        Vec2 next_pos = Vec2{900.0f, 400.0f};
        f32 spacing = 45.0f + 10.0f;
        for(u32 particle_index = 0; particle_index < 4; particle_index++)
        {
            Particle *p = spring_box->p + particle_index;
            f32 dx = spacing;
            f32 dy = 0;
            if(particle_index % 2 == 0)
            {
                dy = spacing;    
                dx = -spacing;
            }
            next_pos += {dx, dy};
            p->pos = next_pos;
            p->r = 10.0f;
            p->inv_m = 1.0f;
            spring_box->t_inv_m += p->inv_m;
            for(u32 i = 1; i <= 3; i++)
            {
                spring_box->adj_list[particle_index][i - 1] = (particle_index + i) % 4;
            }
        }
    }

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
        float w1 = A1 * sinf(2*PI * f1 * total_time + 1.2f);
        float w2 = A2 * sinf(2*PI * f2 * total_time + 4.3f);
        float w3 = A3 * sinf(2*PI * f3 * total_time + 2.7f);

        float dw = theta * (0.0f - windState) * dt
                + sigma * sqrtf(dt) * RandomNormal();  
        windState += dw;        
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
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "displacement: %.2f, %.2f";
            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, spr_disp.x, spr_disp.y);
            push_text(render_group, &state->font_info, at, 15, 90);

        }


        process_physics(particles, count, dt);
        for(u32 i = 0; i < count - 1; i++)
        {
            Particle *p = particles + i;
            push_circle(render_group, p->pos.x, p->pos.y, p->r);
        }

        f32 fuck_off = 2400.0f;
        if(input_is_key_pressed(&g_input, Keys_W))
        {
            push_force += {0.0f, -fuck_off};
        }
        if(input_is_key_pressed(&g_input, Keys_S))
        {
            push_force += {0.0f, fuck_off};
        }
        if(input_is_key_pressed(&g_input, Keys_A))
        {
            push_force += {-fuck_off, 0.0f};
        }
        if(input_is_key_pressed(&g_input, Keys_D))
        {
            push_force += {fuck_off, 0.0f};
        }


        {
            {
                // cm spring box
                spring_box->cm = {}; 
                Vec2 sum_of_ps = {};
                for(u32 particle_index = 0; particle_index < 4; particle_index++)
                {
                    Particle *p = spring_box->p + particle_index;
                    sum_of_ps += p->pos; 
                }
                spring_box->cm = sum_of_ps * spring_box->t_inv_m; 
            }

            {
                // test
                Vec2 a = spring_force(Vec2{10.0f, 15.0f}, Vec2{100.0f, 50.0f}, 10.0f, 13.0f);
                Vec2 b = spring_force(Vec2{100.0f, 50.0f}, Vec2{10.0f, 15.0f}, 10.0f, 13.0f);
                int x = 10;
            }

            #if 0
            {
                // Ref: pnwe1
                // physics spring
                spring_box->k = 1500.0f;
                Vec2 system_forces = {};
                Vec2 g = {0.0f, 9.8f * pixels_per_meter};
                for(u32 particle_index = 0; particle_index < 4; particle_index++)
                {
                    Particle *p = spring_box->p + particle_index;
                    Vec2 f = {};
                    Vec2 anchor1 = (spring_box->p+spring_box->adj_list[particle_index][0])->pos;
                    Vec2 anchor2 = (spring_box->p+spring_box->adj_list[particle_index][1])->pos;
                    Vec2 anchor3 = (spring_box->p+spring_box->adj_list[particle_index][2])->pos;
                    f += spring_force(anchor1, p->pos, 20.0f, spring_box->k);
                    f += spring_force(anchor2, p->pos, 20.0f, spring_box->k);
                    f += spring_force(anchor3, p->pos, 20.0f, spring_box->k);
                    {
                        if(len_sq(p->vel) > 0.0f)
                        {
                            f32 d_k = 0.003f;
                            Vec2 drag_dir = norm(p->vel) * -1.0f;
                            f32 drag_mag = d_k * len_sq(p->vel);
                           f += drag_dir * drag_mag;
                        }
                    }
                    //f += grav_attrac_force(Vec2{900.0f + 20.0f, 400.0f + 20.0f}, p->pos, 50.0f, spring_box->k);
                    {
                        // two masses, m, and m' a distance d apart.
                        Vec2 d = p->pos - spring_box->cm;
                        //f += g.y * (p->pos * 1000.0f) *  (1.0f / len_sq(d));

                    }
                    Vec2 wind = {w1 + w2 + w3, 0.0f};
                    //p->f += 0.005f * p->vel; // some linear damping!
                    p->acc = p->f * p->inv_m;
                    p->acc += g;
                    p->vel += p->acc * dt;
                    p->pos += p->vel * dt;

                    // collisions
                    if (p->pos.x + p->r > SRC_WIDTH)
                    {
                        p->pos.x = SRC_WIDTH - p->r;
                        p->vel.x *= -0.4f;
                    }
                    if (p->pos.x - p->r < 0)
                    {
                        p->pos.x = p->r;
                        p->vel.x *= -0.4f;
                    }
                    if (p->pos.y + p->r > SRC_HEIGHT)
                    {
                        p->pos.y = SRC_HEIGHT - p->r;
                        p->vel.y *= -0.4f;
                    }
                    if (p->pos.y - p->r < 0)
                    {
                        p->pos.y = p->r;
                        p->vel.y *= -0.4f;
                    }
                }

            }
            #endif

            #if 1
            {
                /* NOTE for this soft body to work I have to apply the same force there is from ab=A to B to pA=ab and pB=-ba 
                So because I wanted to have everything in the same loop, the second pass I did in the update of the springs would have a different position
                for the particle A because it would have had its integration done, so the forces werent on opposite direction, the were opposites all together!

                // See `Ref: pnwe1` for the non-working code
                */
                // physics spring
                spring_box->k = 1500.0f;
                Vec2 system_forces = {};
                Vec2 g = {0.0f, 9.8f * pixels_per_meter};
                for(u32 particle_index = 0; particle_index < 4; particle_index++)
                {
                    Particle *p = spring_box->p + particle_index;
                    p->f = {};
                    Vec2 anchor1 = (spring_box->p+spring_box->adj_list[particle_index][0])->pos;
                    Vec2 anchor2 = (spring_box->p+spring_box->adj_list[particle_index][1])->pos;
                    Vec2 anchor3 = (spring_box->p+spring_box->adj_list[particle_index][2])->pos;
                    p->f += spring_force(anchor1, p->pos, 40.0f, spring_box->k);
                    p->f += spring_force(anchor2, p->pos, 40.0f, spring_box->k);
                    p->f += spring_force(anchor3, p->pos, 40.0f, spring_box->k);

                    {
                        if(len_sq(p->vel) > 0.0f)
                        {
                            f32 d_k = 0.0032f;
                            Vec2 drag_dir = norm(p->vel) * -1.0f;
                            f32 drag_mag = d_k * len_sq(p->vel);
                            p->f += drag_dir * drag_mag;
                        }
                    }
                    //f += grav_attrac_force(Vec2{900.0f + 20.0f, 400.0f + 20.0f}, p->pos, 50.0f, spring_box->k);
                    {
                        // two masses, m, and m' a distance d apart.
                        Vec2 d = p->pos - spring_box->cm;
                        //f += g.y * (p->pos * 1000.0f) *  (1.0f / len_sq(d));

                    }
                    //p->f += push_force;
                }
                for(u32 particle_index = 0; particle_index < 4; particle_index++)
                {
                    Particle *p = spring_box->p + particle_index;
                    Vec2 wind = {w1 + w2 + w3, 0.0f};
                    p->acc = p->f * p->inv_m;
                    p->acc += g;
                    p->vel += p->acc * dt;
                    p->vel += push_force;
                    p->pos += p->vel * dt;

                    // collisions
                    if (p->pos.x + p->r > SRC_WIDTH)
                    {
                        p->pos.x = SRC_WIDTH - p->r;
                        p->vel.x *= -0.9f;
                    }
                    if (p->pos.x - p->r < 0)
                    {
                        p->pos.x = p->r;
                        p->vel.x *= -0.9f;
                    }
                    if (p->pos.y + p->r > SRC_HEIGHT)
                    {
                        p->pos.y = SRC_HEIGHT - p->r;
                        p->vel.y *= -0.9f;
                    }
                    if (p->pos.y - p->r < 0)
                    {
                        p->pos.y = p->r;
                        p->vel.y *= -0.9f;
                    }

                }
            }
            #endif 

            {
                // render spring boxxxx
                for(u32 particle_index = 0; particle_index < 4; particle_index++)
                {
                    Particle *p = spring_box->p + particle_index;
                    Vec2 anchor1 = (spring_box->p+spring_box->adj_list[particle_index][0])->pos;
                    Vec2 anchor2 = (spring_box->p+spring_box->adj_list[particle_index][1])->pos;
                    Vec2 anchor3 = (spring_box->p+spring_box->adj_list[particle_index][2])->pos;

                    push_circle(render_group, p->pos, p->r);
                    push_line_2d(render_group, p->pos, anchor1, 1.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
                    push_line_2d(render_group, p->pos, anchor2, 1.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
                    push_line_2d(render_group, p->pos, anchor3, 1.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
                }
            }
        }


        {
            // system of springs

            Vec2 system_forces = {};
            Vec2 g = Vec2{0.0f, pixels_per_meter * 9.8f};
            //system_forces += Vec2{pixels_per_meter * 0.2f, 0.0f};
            f32 system_total_internal_forces = 0.0f;

            // process system
            for (u32 i = 0; i < 16; i++)
            {
                Vec2 *p = &system->p[i];
                Vec2 *v = &system->v[i];
                f32 r = system->r[i];

                Vec2 f = system_forces;
                Vec2 a = f;
                if (i > 3)
                {
                    if(i < 7)
                    {
                        Vec2 anchor = system->p[i - 3];
                        f += spring_force(anchor, *p, 50.0f, system->k);
                    }
                    Vec2 anchor = system->p[i - 4];
                    f += spring_force(anchor, *p, 50.0f, system->k);
                    //f += Vec2{50.0f * sin(2.0f * 3.14f * 0.2f * total_time) , 0.0f};
                    Vec2 wind = {w1 + w2 + w3};
                    wind = {windState, 0.0f};
                    //f += wind;
                    {
                        if(len_sq(*v) > 0.0f)
                        {
                            f32 d_k = 0.221f;
                            Vec2 drag_dir = norm(*v) * -1.0f;
                            f32 drag_mag = d_k * len_sq(*v);
                            f += drag_dir * drag_mag;
                        }
                    }
                    a = f * system->inv_m[i];
                    a += g;
                }
                *v += a * dt;
                *p += *v * dt;
                
                if (p->x + r > SRC_WIDTH)
                {
                    p->x = SRC_WIDTH - r;
                    v->x *= -1.0f;
                }
                if (p->x - r < 0)
                {
                    p->x = r;
                    v->x *= -1.0f;
                }
                if (p->y + r > SRC_HEIGHT)
                {
                    p->y = SRC_HEIGHT - r;
                    v->y *= -0.3f;
                }
                if (p->y - r < 0)
                {
                    p->y = r;
                    v->y *= -0.3f;
                }
            }

            // render system
            {
                aim_profiler_time_block("fantasy");
                for (u32 i = 0; i < 16; i+=4)
                {
                    Vec2 pi = system->p[i];
                    Vec2 pj = system->p[i + 1];
                    Vec2 pk = system->p[i + 2];
                    Vec2 pw = system->p[i + 3];
                    f32 ri = system->r[i];
                    f32 rj = system->r[i + 1];
                    f32 rk = system->r[i + 2];
                    f32 rw = system->r[i + 3];
                    push_circle(render_group, pi, ri);
                    push_circle(render_group, pj, rj);
                    push_circle(render_group, pk, rk);
                    push_circle(render_group, pw, rw);

                    // horiz lines
                    #if 0
                    push_line_2d(render_group, pi, pj, 1.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
                    push_line_2d(render_group, pj, pk, 1.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
                    push_line_2d(render_group, pk, pw, 1.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
                    #endif

                    if (i == 12) continue;
                    // vert lines
                    push_line_2d(render_group, pi, system->p[i + 4], 1.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
                    push_line_2d(render_group, pj, system->p[i + 4 + 1], 1.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
                    push_line_2d(render_group, pk, system->p[i + 4 + 2], 1.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
                    push_line_2d(render_group, pw, system->p[i + 4 + 3], 1.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
                }
            }
        }

        // spring
        // Hooke's law: F = -kx
        f32 k = 300.5f;
        f32 spr_inc = 0.8f;
        Particle *chosen_part = particles + (count - 1);
        if(input_is_key_pressed(&g_input, Keys_Q))
        {
            g_w32_window.is_running = false;
        }
        if(input_is_key_pressed(&g_input, Keys_Space))
        {
            if (spr_start.y + spr_disp.y <= spr_start.y - 30.0f)
            {

            }
            else
            {
                spr_disp += {0.0f, spr_inc};
            }

        }
        else
        {
            {
                #if 0
                #endif
            }
            if (input_was_key_pressed(&g_input, Keys_Space) && input_is_key_released(&g_input, Keys_Space))
            {
                apply_spr = 1;
                spring_activated = 1;

                Vec2 spr_f = {};
                spr_f = spr_disp * k;
                ch_p_f = {};
                ch_p_f += Vec2{ pixels_per_meter * 0.2f, 0.0f };
                ch_p_f += spr_f;
            }
            if(apply_spr == 1)
            {
                // NOTE the same spring force must be applied to the line
                if (chosen_part->pos.y < spr_eq.y)
                {
                    spr_disp.y = spr_eq.y - chosen_part->pos.y;
                }
                else
                {
                    apply_spr = 0;
                }
            }
        }
        push_rect(render_group, spr_p.x, spr_p.y, spr_h_w * 2, spr_h_h * 2, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        {
            Vec2 g = Vec2{0.0f, pixels_per_meter * 9.8f};

            Vec2 aux = {spr_start.x, spr_start.y};
            ch_p_f = spring_force(aux, chosen_part->pos, spr_rest_length, k);
            {
                if(len_sq(chosen_part->vel) > 0.0f)
                {
                    f32 d_k = 0.021f;
                    Vec2 drag_dir = norm(chosen_part->vel) * -1.0f;
                    f32 drag_mag = d_k * len_sq(chosen_part->vel);
                    ch_p_f += drag_dir * drag_mag;
                }
            }
            chosen_part->acc = ch_p_f * chosen_part->inv_m;
            chosen_part->acc += g;
            chosen_part->vel += chosen_part->acc * dt;
            chosen_part->pos += chosen_part->vel * dt;
            Particle *p = chosen_part;
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

        spr_end = spr_eq - spr_disp;

        if (spring_activated == 0)
            spr_end = chosen_part->pos;
        // spring drawing
        push_line_2d(render_group, spr_start, spr_end, 1.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
        push_circle(render_group, chosen_part->pos, chosen_part->r);

        {
            opengl->glUseProgram(opengl->program_id);
            opengl->glUniformMatrix4fv(opengl->proj, 1, GL_FALSE, &ortho_proj_mat[0][0]);
            opengl->glUniform1i(opengl->texture_sampler, 0);
            render_frame(opengl, render_group);
        }

        push_force = {};
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