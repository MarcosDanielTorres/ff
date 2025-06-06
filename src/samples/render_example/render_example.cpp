#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>

#define RAW_INPUT 1
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>


// assimp
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>

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


#include "bindings/opengl_bindings.cpp"
#include "camera.h"
#define AIM_PROFILER
#include "aim_timer.h"
#include "aim_profiler.h"


#include "camera.cpp"
#include "aim_timer.cpp"
#include "aim_profiler.cpp"

#include "components.h"
using namespace aim::Components;

#if 0 
float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};
#endif



struct TestFB
{
    GLuint handle;
    GLuint handle_rotated;
    GLuint handle_selection;

    GLuint textureColorbuffer;
    GLuint textureColorbuffer_rot;
    GLuint selection_tex;

    GLuint vbo, vao, ebo;
    GLuint vbo_rot, vao_rot, ebo_rot;

    u32 pid;
};

global_variable TestFB test_fb;

static Arena g_arena;
static Arena g_transient_arena;

// TODO move away from here!
struct TextureQuadVertex
{
    glm::vec3 p;
    glm::vec2 uv;
    glm::vec4 c;
};

struct UIRenderGroup
{
    TextureQuadVertex *vertex_array;
    u32 vertex_count;
    u16 *index_array;
    u32 index_count;
};

struct UIState
{
    GLint proj;
    GLint texture_sampler;
    u32 program_id; 

    GLuint vbo, vao, ebo;
    GLuint tex;
    // TODO probably should have its own memory. After I remove std first!
    FontInfo font_info;
    glm::mat4 ortho_proj_mat;
    UIRenderGroup *render_group;
};


struct InstancesHolder;
struct PlaceholderState
{
    // TODO probably should have its own memory. After I remove std first!
    glm::mat4 persp_proj;
    //ModelInstance *instances;
    InstancesHolder *instances;


    // program ids
    u32 nonskinned_pid;
    u32 skinned_pid;
    u32 transform_compute_shader_pid;
    u32 matrix_compute_shader_pid;

    u32 static_mesh_pid;
    UIRenderGroup *static_mesh_render_group;
    u32 vbo, vao, ebo;
    GLint proj_loc;
    GLint view_loc;
    GLint texture_sampler_loc;
};

#include "renderer/opengl_renderer.cpp"

struct UniformBuffer
{
    size_t mBufferSize = 0;
    GLuint mUboBuffer = 0;
    void init(OpenGL *opengl, size_t bufferSize)
    {
        mBufferSize = bufferSize;

        opengl->glGenBuffers(1, &mUboBuffer);

        opengl->glBindBuffer(GL_UNIFORM_BUFFER, mUboBuffer);
        opengl->glBufferData(GL_UNIFORM_BUFFER, mBufferSize, nullptr, GL_STATIC_DRAW);
        opengl->glBindBuffer(GL_UNIFORM_BUFFER, 0);

    }

    void uploadUboData(OpenGL *opengl, std::vector<glm::mat4> bufferData, int bindingPoint)
    {  
        if (bufferData.empty()) 
        {
            return;
        }
        size_t bufferSize = bufferData.size() * sizeof(glm::mat4);
        opengl->glBindBuffer(GL_UNIFORM_BUFFER, mUboBuffer);
        opengl->glBufferSubData(GL_UNIFORM_BUFFER, 0, bufferSize, bufferData.data());
        opengl->glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, mUboBuffer, 0, bufferSize);
        opengl->glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void cleanup(OpenGL *opengl)
    {
        opengl->glDeleteBuffers(1, &mUboBuffer);
    }
};

struct ShaderStorageBuffer
{
    size_t mBufferSize = 0;
    GLuint mShaderStorageBuffer = 0;

    void init(OpenGL *opengl, size_t buffer_size)
    {
        mBufferSize = buffer_size;
        opengl->glGenBuffers(1, &mShaderStorageBuffer);
        opengl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
        opengl->glBufferData(GL_SHADER_STORAGE_BUFFER, mBufferSize, nullptr, GL_DYNAMIC_COPY);
        opengl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    // uploads and bind
    template <typename T>
    void uploadSsboData(OpenGL *opengl, std::vector<T> bufferData, int bindingPoint) 
    {
        if (bufferData.empty()) {
            return;
        }

        size_t bufferSize = bufferData.size() * sizeof(T);
        if (bufferSize > mBufferSize) {
            //Logger::log(1, "%s: resizing SSBO %i from %i to %i bytes\n", __FUNCTION__, mShaderStorageBuffer, mBufferSize, bufferSize);
            printf("%s: resizing SSBO %i from %zi to %zi bytes\n", __FUNCTION__, mShaderStorageBuffer, mBufferSize, bufferSize);
            cleanup(opengl);
            init(opengl, bufferSize);
        }

        opengl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
        opengl->glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, bufferData.data());
        opengl->glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingPoint, mShaderStorageBuffer, 0,
            bufferSize);
        opengl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    // just upalods, no bind
    template <typename T> 
    void uploadSsboData(OpenGL *opengl, std::vector<T> bufferData) 
    {
        if (bufferData.empty()) {
            return;
        }

        size_t bufferSize = bufferData.size() * sizeof(T);
        if (bufferSize > mBufferSize) {
            //Logger::log(1, "%s: resizing SSBO %i from %i to %i bytes\n", __FUNCTION__, mShaderStorageBuffer, mBufferSize, bufferSize);
            printf("%s: resizing SSBO %i from %zi to %zi bytes\n", __FUNCTION__, mShaderStorageBuffer, mBufferSize, bufferSize);
            cleanup(opengl);
            init(opengl, bufferSize);
        }

        opengl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
        opengl->glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, bufferData.data());
        opengl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void bind(OpenGL *opengl, int bindingPoint) 
    {
        if (mBufferSize == 0) 
        {
            return;
        }

        opengl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
        opengl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, mShaderStorageBuffer);
        opengl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void checkForResize(OpenGL *opengl, size_t newBufferSize) 
    {
        if (newBufferSize > mBufferSize) 
        {
            printf:("%s: resizing SSBO %i from %i to %i bytes\n", __FUNCTION__, mShaderStorageBuffer, mBufferSize, newBufferSize);
            cleanup(opengl);
            init(opengl, newBufferSize);
        }
    }

    void cleanup(OpenGL *opengl)
    {
        opengl->glDeleteBuffers(1, &mShaderStorageBuffer);
    }
};
#include "model_loader.cpp"
#include "renderer.cpp"

///////// TODO cleanup ////////////
// global state
// window size


#include "shader.h"

global_variable OS_Window global_w32_window;
global_variable u32 os_modifiers;
global_variable GameInput global_input;

global_variable UniformBuffer mUniformBuffer{};

/* for non-animated models */
global_variable std::vector<glm::mat4> mWorldPosMatrices{};

// removido se mete por mShaderModelRootMatrixBuffer
// global_variable ShaderStorageBuffer mWorldPosBuffer{};

/* for animated models */
global_variable ShaderStorageBuffer mShaderBoneMatrixBuffer{};
global_variable ShaderStorageBuffer mShaderModelRootMatrixBuffer;
/* for compute shaders */
global_variable ShaderStorageBuffer mShaderTRSMatrixBuffer;
global_variable ShaderStorageBuffer mNodeTransformBuffer;
std::vector<NodeTransformData> mNodeTransFormData{};

/* for selection */
std::vector<u32> ent_ids_data{};
global_variable ShaderStorageBuffer entities_ids;
global_variable Shader outline_shader;


// mAssimpShader.use();

global_variable u32 SRC_WIDTH = 1680;
global_variable u32 SRC_HEIGHT = 945;

// TODO: Removed these
// used during mouse callback
global_variable float lastX = SRC_WIDTH / 2.0f;
global_variable float lastY = SRC_HEIGHT / 2.0f;
global_variable bool firstMouse = true;

// lighting
glm::vec3 light_pos(1.2f, 1.0f, 2.0f);
glm::vec3 light_dir(-0.2f, -1.0f, -0.3f);
glm::vec3 light_scale(0.2f);

glm::vec3 light_ambient(0.2f, 0.2f, 0.2f);
glm::vec3 light_diffuse(0.5f, 0.5f, 0.5f); // darken diffuse light a bit
glm::vec3 light_specular(1.0f, 1.0f, 1.0f);

// rifle
glm::vec3 rifle_pos(-2.2f, 0.0f, 0.0f);
glm::vec3 rifle_rot(0.0f, 0.0f, 0.0f);
float rifle_scale(0.5f);

// manny
glm::vec3 manny_pos(-2.2f, 0.0f, 0.0f);
glm::vec3 manny_rot(0.0f, 0.0f, 0.0f);
float manny_scale(0.07f);

// model
glm::vec3 floor_pos(0.0f, 0.0f, -2.2f);
glm::vec3 floor_scale(80.0f, 1.0f, 80.0f);
glm::vec3 model_bounding_box(floor_scale * 1.0f); 

// model materials
glm::vec3 model_material_ambient(1.0f, 0.5f, 0.31f);
glm::vec3 model_material_diffuse(1.0f, 0.5f, 0.31f);
glm::vec3 model_material_specular(0.5f, 0.5f, 0.5f);
float model_material_shininess(32.0f);

static bool gui_mode = false;
static bool fps_mode = false;

Camera free_camera(FREE_CAMERA, glm::vec3(0.0f, 5.0f, 19.0f));
Camera fps_camera(FPS_CAMERA, glm::vec3(0.0f, 8.0f, 3.0f));

global_variable Camera* curr_camera = &free_camera;
///////// TODO cleanup ////////////

struct MeshBox {
	Transform3D transform;
	MeshBox(Transform3D transform) {
		this->transform = transform;
	}
};


struct PointLight {
	Transform3D transform;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	float constant;
	float linear;
	float quadratic;
};

struct SpotLight {
	Transform3D transform;
	glm::vec3 direction;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	float constant;
	float linear;
	float quadratic;

	float cutOff;
	float outerCutOff;
};

struct DirectionalLight {
	glm::vec3 direction;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};


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
                if(xPos >= 0 && xPos < SRC_WIDTH && yPos >= 0 && yPos < SRC_HEIGHT)
                {
                }
                // comente esto
                //SetCapture(global_w32_window.handle);

                i32 xxPos = LOWORD(Message.lParam);
                i32 yyPos = HIWORD(Message.lParam);
                char buf[100];
                sprintf(buf,  "MOUSE MOVE: x: %d, y: %d\n", xPos, yPos);
                //printf(buf);

                Assert((xxPos == xPos && yyPos == yPos));
                global_input.curr_mouse_state.x = xPos;
                global_input.curr_mouse_state.y = yPos;
                
                #if !defined(RAW_INPUT)
                global_input.dx = global_input.curr_mouse_state.x - global_input.prev_mouse_state.x;
                global_input.dy = -(global_input.curr_mouse_state.y - global_input.prev_mouse_state.y);
                #endif

            }
            break;
            #if RAW_INPUT
            case WM_INPUT: {
                UINT size;
                GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

                AssertGui(size < 1000, "GetRawInputData surpassed 1000 bytes");
                u8 bytes[1000];
                RAWINPUT* raw = (RAWINPUT*)bytes;
                GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER));
                if (!(raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
                {
                    if (raw->header.dwType == RIM_TYPEMOUSE) 
                    {
                        LONG dx = raw->data.mouse.lLastX;
                        LONG dy = raw->data.mouse.lLastY;
                        global_input.dx = f32(dx);
                        global_input.dy = f32(-dy);
                    }
                }else
                {
                    AssertGui(1 < 0, "MOUSE_MOVE_ABSOLUTE");
                }
                POINT center = { LONG(SRC_WIDTH)/2, LONG(SRC_HEIGHT)/2 };
                ClientToScreen(global_w32_window.handle, &center);
                SetCursorPos(center.x, center.y);
                //printf("RECENTERING!!!\n");

            } break;
            #endif
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
    #if RAW_INPUT
    if (global_w32_window.focused)
    {
        if (global_input.prev_mouse_state.x != SRC_WIDTH / 2 ||
            global_input.prev_mouse_state.y != SRC_HEIGHT / 2)
        {
            POINT pos = {i32(SRC_WIDTH / 2), i32(SRC_HEIGHT / 2)};
            ClientToScreen(global_w32_window.handle, &pos);
            SetCursorPos(pos.x, pos.y);
            //printf("RECENTERING!!!\n");
        }
    }
    #endif
}

LRESULT CALLBACK win32_main_callback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    switch(Message)
    {
        case WM_DESTROY:
        {
            global_w32_window.is_running = false;
        } break;
        case WM_SIZE:
        {
            // os get window dimensions
            RECT r;
            GetClientRect(global_w32_window.handle, &r);
            u32 width = r.right - r.left;
            u32 height = r.bottom - r.top;
            if (width != SRC_WIDTH || height != SRC_HEIGHT)
            {
                //SRC_WIDTH = width;
                //SRC_HEIGHT = height;
                glViewport(0, 0, width, height);
            }
        }break;
        #if RAW_INPUT
        case WM_KILLFOCUS:
        {
            global_w32_window.focused = false;
            //while( ShowCursor(TRUE) < 0 );
            printf("KILL FOCUS\n");
            RAWINPUTDEVICE rid = { 0x01, 0x02, RIDEV_REMOVE, NULL };

            if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
            {
                Assert('wtf');
            }
            ClipCursor(NULL);
            ReleaseCapture();
            ShowCursor(1);
        }break;
        case WM_SETFOCUS:
        {
            global_w32_window.focused = true;
            ShowCursor(0);
            //while( ShowCursor(FALSE) >= 0 );
            //POINT center = { LONG(SRC_WIDTH)/2, LONG(SRC_HEIGHT)/2 };
            //ClientToScreen(global_w32_window.handle, &center);
            //SetCursorPos(center.x, center.y);

            printf("SET FOCUS\n");
            RAWINPUTDEVICE rid = {
                .usUsagePage = 0x01,                   // generic desktop controls
                .usUsage     = 0x02,                   // mouse
                //.dwFlags     = RIDEV_INPUTSINK       // receive even when not focused
                //            | RIDEV_NOLEGACY,      // no WM_MOUSEMOVE fallback
                .dwFlags = 0,
                .hwndTarget  = global_w32_window.handle,
            };

            if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
                // failure!
            }
            //RECT clipRect;
            //GetClientRect(Window, &clipRect);
            //ClientToScreen(Window, (POINT*) &clipRect.left);
            //ClientToScreen(Window, (POINT*) &clipRect.right);
            //ClipCursor(&clipRect);

            //RECT ja;
            //GetWindowRect(Window, &ja);
            //ClipCursor(&ja);
        }break;
        #endif
        //case WM_PAINT: {
        //    PAINTSTRUCT Paint;
        //    HDC DeviceContext = BeginPaint(Window, &Paint);
        //    OS_Window_Dimension Dimension = os_win32_get_window_dimension(Window);
        //    os_win32_display_buffer(DeviceContext, &global_pixel_buffer,
        //                               Dimension.width, Dimension.height);
        //    EndPaint(Window, &Paint);
        //}break;
        default:
        {
            result = DefWindowProcA(Window, Message, wParam, lParam);
        } break;
    }
    return result;
}






internal
void update_and_render(PlaceholderState *state)
{

}


/*

render_entry_quads -> shader
render_entry_models -> shader

I don't know what happens if entries are interleaved i guess i should maximize not context switching the shaders!
this is a fucking to do!

this is for rendering only and its only useful to know which shader and config you need, nothing else
for each entry:
    switch entry->type
        render_entry_quads:
        render_entry_models:
        render_entry_static:
        ...
*/


internal
void fb_fuckery(OpenGL *opengl, TestFB *fb, GLsizei width, GLsizei height)
{

    /*
    Now that we know how framebuffers (sort of) work it's time to put them to good use.
    We're going to render the scene into a color texture attached to a framebuffer object 
    we created and then draw this texture over a simple quad that spans the whole screen.
    */
    opengl->glGenFramebuffers(1, &fb->handle);
    opengl->glBindFramebuffer(GL_FRAMEBUFFER, fb->handle);

    // generate texture
    glGenTextures(1, &fb->textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, fb->textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // attach it to currently bound framebuffer object
    opengl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->textureColorbuffer, 0); 

    /*
    We also want to make sure OpenGL is able to do depth testing (and optionally stencil testing)
    so we have to make sure to add a depth (and stencil) attachment to the framebuffer.
    Since we'll only be sampling the color buffer and not the other buffers we can create a renderbuffer object
    for this purpose.
    */
    GLuint rbo;
    opengl->glGenRenderbuffers(1, &rbo);
    opengl->glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    opengl->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    opengl->glBindRenderbuffer(GL_RENDERBUFFER, 0);

    opengl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    if(opengl->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n");


    // selection texture
    // generate texture
    glGenTextures(1, &fb->selection_tex);
    glBindTexture(GL_TEXTURE_2D, fb->selection_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // attach it to currently bound framebuffer object
    opengl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fb->selection_tex, 0); 

    const GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    opengl->glDrawBuffers(2, buffers);

    /*
    Now that we know how framebuffers (sort of) work it's time to put them to good use.
    We're going to render the scene into a color texture attached to a framebuffer object 
    we created and then draw this texture over a simple quad that spans the whole screen.
    */
    opengl->glGenFramebuffers(1, &fb->handle_rotated);
    opengl->glBindFramebuffer(GL_FRAMEBUFFER, fb->handle_rotated);

    int mirrorWidth = 320;
    int mirrorHeight = 180;
    // generate texture
    glGenTextures(1, &fb->textureColorbuffer_rot);
    glBindTexture(GL_TEXTURE_2D, fb->textureColorbuffer_rot);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mirrorWidth, mirrorHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // attach it to currently bound framebuffer object
    opengl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->textureColorbuffer_rot, 0); 

    /*
    We also want to make sure OpenGL is able to do depth testing (and optionally stencil testing)
    so we have to make sure to add a depth (and stencil) attachment to the framebuffer.
    Since we'll only be sampling the color buffer and not the other buffers we can create a renderbuffer object
    for this purpose.
    */
    GLuint rbo_rot;
    opengl->glGenRenderbuffers(1, &rbo_rot);
    opengl->glBindRenderbuffer(GL_RENDERBUFFER, rbo_rot);
    opengl->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mirrorWidth, mirrorHeight);
    opengl->glBindRenderbuffer(GL_RENDERBUFFER, 0);

    opengl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_rot);
    if(opengl->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n");

    opengl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// NOTE before pulling them out I have to sort out the globals being used here!
void opengl_render(OpenGL *opengl, Camera *curr_camera, u32 cubeVAO,
    Shader skinning_shader, MeshBox floor_meshbox, std::vector<MeshBox> boxes, PlaceholderState *placeholder_state, UIState *ui_state, f32 dt)
{
    glm::mat4 view = curr_camera->GetViewMatrix();
    f32 mouse_p_x = global_input.curr_mouse_state.x;
    f32 mouse_p_y = global_input.curr_mouse_state.y;

    glm::vec3 rear_forward = -curr_camera->forward;
    glm::mat4 view_rotated_180 = glm::lookAt(curr_camera->position, curr_camera->position + rear_forward, curr_camera->up);
    int mirrorWidth = 320;
    int mirrorHeight = 180;
    local_persist f32 pixelColor =  21.0f;
    local_persist unsigned char stencil_val;

    {
        char buf[100];
        char *at = buf;
        char *end = buf + sizeof(buf);
        const char* c  = "ID: %2.f";
        
        f32 mouse_p_x = global_input.curr_mouse_state.x;
        f32 mouse_p_y = global_input.curr_mouse_state.y;
        _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, pixelColor);
        push_text(ui_state, at, 500, 100, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    }

    {
        char buf[100];
        char *at = buf;
        char *end = buf + sizeof(buf);
        const char* c  = "stencil value at mouse: %u";
        _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, stencil_val);
        push_text(ui_state, at, 600, 120, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        
    }
    {
        char buf[100];
        char *at = buf;
        char *end = buf + sizeof(buf);
        const char* c  = "fbo bound: %d";
        _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, test_fb.handle);
        push_text(ui_state, at, 600, 130) ;
        
    }




    {
        // first pass
        opengl->glBindFramebuffer(GL_FRAMEBUFFER, test_fb.handle);
        glViewport(0, 0, SRC_WIDTH, SRC_HEIGHT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glEnable(GL_STENCIL_TEST);
        glDepthFunc(GL_LESS);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        const GLenum all_colors[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        opengl->glDrawBuffers(2, all_colors);

        //static GLfloat colorClear[] = { 0.25f, 0.25f, 0.25f, 1.0f };
        static GLfloat colorClear[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        opengl->glClearBufferfv(GL_COLOR, 0, colorClear);
        static GLfloat selectionClearColor = 0xFFFFFFFF;
        opengl->glClearBufferfv(GL_COLOR, 1, &selectionClearColor);
        static GLfloat depthValue = 1.0f;
        opengl->glClearBufferfv(GL_DEPTH, 0, &depthValue);
        glStencilMask(0xFF);
        glClear(GL_STENCIL_BUFFER_BIT);            // Clear stencil

        //glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glStencilMask(0x00);

	    opengl->glBindVertexArray(cubeVAO);
        shader_use(skinning_shader);

        shader_set_mat4(skinning_shader, "nodeMatrix", glm::mat4(1.0f));
		shader_set_mat4(skinning_shader, "view", view);
        shader_set_vec3(skinning_shader, "viewPos", curr_camera->position);
        shader_set_vec3(skinning_shader, "spotLight.position", curr_camera->position);
        shader_set_vec3(skinning_shader, "spotLight.direction", curr_camera->forward);

		glm::mat4 model_mat = glm::translate(glm::mat4(1.0f), floor_meshbox.transform.pos) *
			glm::mat4_cast(floor_meshbox.transform.rot) *
			glm::scale(glm::mat4(1.0f), floor_meshbox.transform.scale);

        opengl->glUniform1f(opengl->glGetUniformLocation(skinning_shader.id, "u_EntityID"), (f32)1);
		shader_set_mat4(skinning_shader, "model", model_mat);
        // NOTE glDrawArrays takes the amount of vertices, not points (vertex is pos, norm, tex, ... so on, a combination of things!!)
        glDrawArrays(GL_TRIANGLES, 0, 36);




        for (u32 i = 3; i < boxes.size(); i++)
        {
            if (pixelColor > 1 && pixelColor < boxes.size())
            {
                u32 entity_id_to_draw = u32(pixelColor);
                if (entity_id_to_draw == i)
                {
                    glEnable(GL_STENCIL_TEST);
                    glStencilFunc(GL_ALWAYS, 1, 0xFF);       // Always pass the stencil test
                    glStencilMask(0xFF);                       // Enable writing to stencil
                    // activar esta hace que cuando mnuestro el valor del stencil buffer sea correcto
                    // la de abajo lo deja siempre en 0! (glStencilOp(GL_KEEP, GL_REPLACE, GL_KEEP)) 
                    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); 
                    // when stencil passes and depth fails
                    // glStencilOp(GL_KEEP, GL_REPLACE, GL_KEEP); 
                    opengl->glUniform1f(opengl->glGetUniformLocation(skinning_shader.id, "u_EntityID"), (f32)i);
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), boxes[i].transform.pos) *
                        glm::mat4_cast(boxes[i].transform.rot) *
                        glm::scale(glm::mat4(1.0f), boxes[i].transform.scale);
                    shader_set_mat4(skinning_shader, "model", model);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);       // Always pass the stencil test
                    // draw outline
                    glStencilMask(0x00); // Disable stencil writes


                    // IMPORTANT This doesn't work
                    #if 0
                    {
                        // draw outline
                        glDisable(GL_DEPTH_TEST);
                        // Use a solid outline color, simple shader
                        shader_use(outline_shader);
                        opengl->glBindVertexArray(cubeVAO);

                        shader_set_mat4(outline_shader, "nodeMatrix", glm::mat4(1.0f));
                        shader_set_mat4(outline_shader, "view", view);
                        shader_set_vec3(outline_shader, "viewPos", curr_camera->position);
                        shader_set_vec3(outline_shader, "spotLight.position", curr_camera->position);
                        shader_set_vec3(outline_shader, "spotLight.direction", curr_camera->forward);
                        shader_set_mat4(outline_shader, "projection", placeholder_state->persp_proj);

                        glm::mat4 outline_model = glm::scale(model, glm::vec3(1.2f));
                        shader_set_mat4(outline_shader, "model", outline_model);
                        glDrawArrays(GL_TRIANGLES, 0, 36);
                        // reset states
                        glStencilMask(0xFF);
                        glStencilFunc(GL_ALWAYS, 0, 0xFF);
                        glEnable(GL_DEPTH_TEST);
                        shader_use(skinning_shader);
                    }
                    #endif

                    continue;
                }
            }

            opengl->glUniform1f(opengl->glGetUniformLocation(skinning_shader.id, "u_EntityID"), (f32)i);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), boxes[i].transform.pos) *
                glm::mat4_cast(boxes[i].transform.rot) *
                glm::scale(glm::mat4(1.0f), boxes[i].transform.scale);
            shader_set_mat4(skinning_shader, "model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }


        opengl->glUseProgram(0);


        const GLenum only_color[] = {GL_COLOR_ATTACHMENT0};
        opengl->glDrawBuffers(1, only_color);
        // IMPORTANT because I only care about the selection for the cubes i have to disable the other color_attachment
        // otherwise if the next shaders dont specifically write to location 1 then whats written its undefined behaviour
        // render model
        {
            InstancesHolder *instances = placeholder_state->instances;
            Model *woman = instances->model;
            std::vector<glm::mat4> matrixData;
            matrixData.emplace_back(view);
            matrixData.emplace_back(placeholder_state->persp_proj);
            mUniformBuffer.uploadUboData(opengl, matrixData, 0);

            #if 0
            opengl->glUseProgram(placeholder_state->nonskinned_pid);
            // generic data for all models!
            //std::vector<glm::mat4> matrixData;
            //matrixData.emplace_back(view);
            //matrixData.emplace_back(placeholder_state->persp_proj);

            //mUniformBuffer.uploadUboData(opengl, matrixData, 0);

            /* non-animated models */
            mWorldPosMatrices.clear();
            glm::mat4 experiment = 
            {
                1.0f, 0.0f, 0.0f, 0.0f, 
                0.0f, 1.0f, 0.0f, 0.0f, 
                0.0f, 0.0f, 1.0f, 0.0f, 
                10.0f, 10.0f, -3.0f, 1.0f, 
            };
            mWorldPosMatrices.emplace_back(experiment);
            mWorldPosMatrices.emplace_back(glm::mat4(1.0f));


            //mAssimpShader.use();
            mWorldPosBuffer.uploadSsboData(opengl, mWorldPosMatrices, 1);
            drawInstanced(woman, opengl, 1);
            #else
            /* animated models */
            //if (!woman->mAnimClips.empty()  && !instances->mInstanceSettings[0].mBoneMatrices.empty()) {
            if (!woman->mAnimClips.empty()  && !woman->mBoneList.empty()) {
                size_t numberOfBones = woman->mBoneList.size();
                mNodeTransFormData.resize(instances->count * numberOfBones);
                mWorldPosMatrices.resize(instances->count);

                for(u32 i = 0; i < instances->count; i++)
                {
                    InstanceSettings *settings = &instances->mInstanceSettings[i];
                    settings->updateAnimation(dt, woman);
                    std::vector<NodeTransformData> instanceNodeTransform = settings->mNodeTransformData;
                    std::copy(
                        instanceNodeTransform.begin(),
                        instanceNodeTransform.end(),
                        mNodeTransFormData.begin() + i * numberOfBones
                    );
                    // Al mInstanceRootMatrix la llama a traves de getWorldTransformMatrix()
                    mWorldPosMatrices.at(i) = settings->mInstanceRootMatrix;
                }

                size_t trsMatrixSize = numberOfBones * instances->count * sizeof(glm::mat4);

                /* we may have to resize the buffers (uploadSsboData() checks for the size automatically, bind() not)*/
                mShaderBoneMatrixBuffer.checkForResize(opengl, trsMatrixSize);
                mShaderTRSMatrixBuffer.checkForResize(opengl, trsMatrixSize);

                /* calculate TRS matrices from node transforms*/
                // use compute shader
                //mAssimpTransformComputeShader.use();
                opengl->glUseProgram(placeholder_state->transform_compute_shader_pid);
                mNodeTransformBuffer.uploadSsboData(opengl, mNodeTransFormData, 0);
                mShaderTRSMatrixBuffer.bind(opengl, 1);

                opengl->glDispatchCompute(numberOfBones, std::ceil(instances->count / 32.0f), 1);
                opengl->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                /* multiply every bone TRS matrix with its parent bones TRS matrices, until the root bone has been reached
                * also, multiply the bone TRS and the bone offset matrix */
                //mAssimpMatrixComputeShader.use();
                opengl->glUseProgram(placeholder_state->matrix_compute_shader_pid);

                mShaderTRSMatrixBuffer.bind(opengl, 0);
                woman->mShaderBoneParentBuffer.bind(opengl, 1);
                woman->mShaderBoneMatrixOffsetBuffer.bind(opengl, 2);
                mShaderBoneMatrixBuffer.bind(opengl, 3);

                /* do the computation - in groups of 32 invocations */
                opengl->glDispatchCompute(numberOfBones, std::ceil(instances->count / 32.0f), 1);
                opengl->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                /* now bind the final bone transforms to the vertex skinning shader */
                opengl->glUseProgram(placeholder_state->skinned_pid);
                opengl->glUniform1i(opengl->glGetUniformLocation(placeholder_state->skinned_pid, "aModelStride"), numberOfBones);

                mShaderBoneMatrixBuffer.bind(opengl, 1);
                mShaderModelRootMatrixBuffer.uploadSsboData(opengl, mWorldPosMatrices, 2);

                drawInstanced(woman, opengl, instances->count);
            }
            #endif
            opengl->glUseProgram(0);
        }

        {
            UIRenderGroup *render_group = placeholder_state->static_mesh_render_group;
            // render ui
            begin_static_mesh_frame(opengl, placeholder_state, render_group);
            opengl->glUniformMatrix4fv(placeholder_state->proj_loc, 1, GL_FALSE, &placeholder_state->persp_proj[0][0]);
            opengl->glUniformMatrix4fv(placeholder_state->view_loc, 1, GL_FALSE, &view[0][0]);

            opengl->glUniform1i(placeholder_state->texture_sampler_loc, 0);

            glDrawElements(GL_TRIANGLES, render_group->index_count, GL_UNSIGNED_SHORT, 0); //era 18
            end_static_mesh_frame(opengl);
        }

                {
            UIRenderGroup *ui_render_group = ui_state->render_group;
            // render ui
            // TODO this should only be done at resizes
            begin_ui_frame(opengl, ui_state, ui_render_group);
            opengl->glUniformMatrix4fv(ui_state->proj, 1, GL_FALSE, &ui_state->ortho_proj_mat[0][0]);
            // TODO see wtf is a 0 there? Why is the texture sampler for?
            opengl->glUniform1i(ui_state->texture_sampler, 0);
            // TODO investigate if i have to set this value for the uniforms again or not!. 

            glDrawElements(GL_TRIANGLES, ui_render_group->index_count, GL_UNSIGNED_SHORT, 0); //era 18
            end_ui_frame(opengl);
        }

    }
        

        // rear view mirror!!!!!!!!!

    #if REAR_VIEW
    {
        // second pass
        opengl->glBindFramebuffer(GL_FRAMEBUFFER, test_fb.handle_rotated);
        glViewport(0, 0, mirrorWidth, mirrorHeight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);



        opengl->glBindVertexArray(cubeVAO);
        shader_use(skinning_shader);

        shader_set_mat4(skinning_shader, "nodeMatrix", glm::mat4(1.0f));
        shader_set_mat4(skinning_shader, "view", view_rotated_180);
        shader_set_vec3(skinning_shader, "viewPos", curr_camera->position);
        shader_set_vec3(skinning_shader, "spotLight.position", curr_camera->position);
        shader_set_vec3(skinning_shader, "spotLight.direction", curr_camera->forward);

        glm::mat4 model_mat = glm::translate(glm::mat4(1.0f), floor_meshbox.transform.pos) *
            glm::mat4_cast(floor_meshbox.transform.rot) *
            glm::scale(glm::mat4(1.0f), floor_meshbox.transform.scale);

        shader_set_mat4(skinning_shader, "model", model_mat);
        // NOTE glDrawArrays takes the amount of vertices, not points (vertex is pos, norm, tex, ... so on, a combination of things!!)
        glDrawArrays(GL_TRIANGLES, 0, 36);
        for (u32 i = 3; i < boxes.size(); i++)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), boxes[i].transform.pos) *
                glm::mat4_cast(boxes[i].transform.rot) *
                glm::scale(glm::mat4(1.0f), boxes[i].transform.scale);
            shader_set_mat4(skinning_shader, "model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // render model
        {
            InstancesHolder *instances = placeholder_state->instances;
            Model *woman = instances->model;
            std::vector<glm::mat4> matrixData;
            matrixData.emplace_back(view_rotated_180);
            matrixData.emplace_back(placeholder_state->persp_proj);
            mUniformBuffer.uploadUboData(opengl, matrixData, 0);

            #if 0
            opengl->glUseProgram(placeholder_state->nonskinned_pid);
            // generic data for all models!
            //std::vector<glm::mat4> matrixData;
            //matrixData.emplace_back(view);
            //matrixData.emplace_back(placeholder_state->persp_proj);

            //mUniformBuffer.uploadUboData(opengl, matrixData, 0);

            /* non-animated models */
            mWorldPosMatrices.clear();
            glm::mat4 experiment = 
            {
                1.0f, 0.0f, 0.0f, 0.0f, 
                0.0f, 1.0f, 0.0f, 0.0f, 
                0.0f, 0.0f, 1.0f, 0.0f, 
                10.0f, 10.0f, -3.0f, 1.0f, 
            };
            mWorldPosMatrices.emplace_back(experiment);
            mWorldPosMatrices.emplace_back(glm::mat4(1.0f));


            //mAssimpShader.use();
            mWorldPosBuffer.uploadSsboData(opengl, mWorldPosMatrices, 1);
            drawInstanced(woman, opengl, 1);
            #else
            /* animated models */
            //if (!woman->mAnimClips.empty()  && !instances->mInstanceSettings[0].mBoneMatrices.empty()) {
            if (!woman->mAnimClips.empty()  && !woman->mBoneList.empty()) {
                size_t numberOfBones = woman->mBoneList.size();
                mNodeTransFormData.resize(instances->count * numberOfBones);
                mWorldPosMatrices.resize(instances->count);

                for(u32 i = 0; i < instances->count; i++)
                {
                    InstanceSettings *settings = &instances->mInstanceSettings[i];
                    settings->updateAnimation(dt, woman);
                    std::vector<NodeTransformData> instanceNodeTransform = settings->mNodeTransformData;
                    std::copy(
                        instanceNodeTransform.begin(),
                        instanceNodeTransform.end(),
                        mNodeTransFormData.begin() + i * numberOfBones
                    );
                    // Al mInstanceRootMatrix la llama a traves de getWorldTransformMatrix()
                    mWorldPosMatrices.at(i) = settings->mInstanceRootMatrix;
                }

                size_t trsMatrixSize = numberOfBones * instances->count * sizeof(glm::mat4);

                /* we may have to resize the buffers (uploadSsboData() checks for the size automatically, bind() not)*/
                mShaderBoneMatrixBuffer.checkForResize(opengl, trsMatrixSize);
                mShaderTRSMatrixBuffer.checkForResize(opengl, trsMatrixSize);

                /* calculate TRS matrices from node transforms*/
                // use compute shader
                //mAssimpTransformComputeShader.use();
                opengl->glUseProgram(placeholder_state->transform_compute_shader_pid);
                mNodeTransformBuffer.uploadSsboData(opengl, mNodeTransFormData, 0);
                mShaderTRSMatrixBuffer.bind(opengl, 1);

                opengl->glDispatchCompute(numberOfBones, std::ceil(instances->count / 32.0f), 1);
                opengl->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                /* multiply every bone TRS matrix with its parent bones TRS matrices, until the root bone has been reached
                * also, multiply the bone TRS and the bone offset matrix */
                //mAssimpMatrixComputeShader.use();
                opengl->glUseProgram(placeholder_state->matrix_compute_shader_pid);

                mShaderTRSMatrixBuffer.bind(opengl, 0);
                woman->mShaderBoneParentBuffer.bind(opengl, 1);
                woman->mShaderBoneMatrixOffsetBuffer.bind(opengl, 2);
                mShaderBoneMatrixBuffer.bind(opengl, 3);

                /* do the computation - in groups of 32 invocations */
                opengl->glDispatchCompute(numberOfBones, std::ceil(instances->count / 32.0f), 1);
                opengl->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                /* now bind the final bone transforms to the vertex skinning shader */
                opengl->glUseProgram(placeholder_state->skinned_pid);
                opengl->glUniform1i(opengl->glGetUniformLocation(placeholder_state->skinned_pid, "aModelStride"), numberOfBones);

                mShaderBoneMatrixBuffer.bind(opengl, 1);
                mShaderModelRootMatrixBuffer.uploadSsboData(opengl, mWorldPosMatrices, 2);

                drawInstanced(woman, opengl, instances->count);
            }
            #endif
            opengl->glUseProgram(0);
        }

        {
            UIRenderGroup *render_group = placeholder_state->static_mesh_render_group;
            // render ui
            begin_static_mesh_frame(opengl, placeholder_state, render_group);
            opengl->glUniformMatrix4fv(placeholder_state->proj_loc, 1, GL_FALSE, &placeholder_state->persp_proj[0][0]);
            opengl->glUniformMatrix4fv(placeholder_state->view_loc, 1, GL_FALSE, &view_rotated_180[0][0]);

            opengl->glUniform1i(placeholder_state->texture_sampler_loc, 0);

            glDrawElements(GL_TRIANGLES, render_group->index_count, GL_UNSIGNED_SHORT, 0); //era 18
            end_static_mesh_frame(opengl);
        }

    }
    #endif

    opengl->glUseProgram(0);

    u32 xPos = (u32)mouse_p_x;
    u32 yPos = (u32)mouse_p_y;
    //opengl->glBindFramebuffer(GL_READ_FRAMEBUFFER, test_fb.handle);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(xPos, yPos, 1, 1, GL_RED, GL_FLOAT, &pixelColor);
    //pixelColor = 21;
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    //opengl->glBindFramebuffer(GL_FRAMEBUFFER, test_fb.handle);


    // stencil pass
    {
        #if 1

        GLint fbo;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
        //printf("FBO at stencil pass: %d\n", fbo);

        GLint stencil_mask;
        glGetIntegerv(GL_STENCIL_WRITEMASK, &stencil_mask);
        //printf("Stencil mask at stencil pass: %x\n", stencil_mask);

        GLboolean depth;
        glGetBooleanv(GL_DEPTH_TEST, &depth);
        //printf("Depth test at stencil pass: %d\n", depth);


        shader_use(skinning_shader);
	    opengl->glBindVertexArray(cubeVAO);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(xPos, yPos, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil_val);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);


        // IMPORTANT this works because i wait for it to be the last thing
        #if 1
        if (pixelColor > 1 && pixelColor < boxes.size())
        {
            u32 entity_to_draw = u32(pixelColor);
            // draw outline
            glDisable(GL_DEPTH_TEST);
            // Use a solid outline color, simple shader
            shader_use(outline_shader);
            opengl->glBindVertexArray(cubeVAO);

            shader_set_mat4(outline_shader, "nodeMatrix", glm::mat4(1.0f));
            shader_set_mat4(outline_shader, "view", view);
            shader_set_vec3(outline_shader, "viewPos", curr_camera->position);
            shader_set_vec3(outline_shader, "spotLight.position", curr_camera->position);
            shader_set_vec3(outline_shader, "spotLight.direction", curr_camera->forward);
            shader_set_mat4(outline_shader, "projection", placeholder_state->persp_proj);

            u32 i = entity_to_draw;
            glm::mat4 outline_model = glm::translate(glm::mat4(1.0f), boxes[i].transform.pos) *
                glm::mat4_cast(boxes[i].transform.rot) *
                glm::scale(glm::mat4(1.0f), boxes[i].transform.scale * 1.2f);

            shader_set_mat4(outline_shader, "model", outline_model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        #endif

        // reset states
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
        glEnable(GL_DEPTH_TEST);

        #endif

    }

    //glReadBuffer(GL_COLOR_ATTACHMENT0);
    opengl->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // composition pass (fb 1 + fb 2)
    // The final buffer doesn't have to get clean because im rendering a quad into it hence overwritting
    // all its pixels. But if the dimensions would be different than the current windows's then I would


    opengl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SRC_WIDTH, SRC_HEIGHT);


    opengl->glUseProgram(test_fb.pid);

    opengl->glBindVertexArray(test_fb.vao);
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, test_fb.textureColorbuffer);
    glDrawArrays(GL_TRIANGLES, 0, 6); 
    opengl->glBindVertexArray(0);

    #if REAR_VIEW
    opengl->glBindVertexArray(test_fb.vao_rot);
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, test_fb.textureColorbuffer_rot);
    glDrawArrays(GL_TRIANGLES, 0, 6); 
    opengl->glBindVertexArray(0);
    #endif
    opengl->glUseProgram(0);
}


int main() {
    aim_profiler_begin();
    global_w32_window = os_win32_open_window("opengl", SRC_WIDTH, SRC_HEIGHT, win32_main_callback, 1);
	
	arena_init(&g_arena, mb(20));
	arena_init(&g_transient_arena, mb(2));

    font_init();
    FontInfo font_info = font_load(&g_arena);

    //os_win32_toggle_fullscreen(global_w32_window.handle, &global_w32_window.window_placement);

    #if RAW_INPUT
    RAWINPUTDEVICE rid = {
        .usUsagePage = 0x01,                   // generic desktop controls
        .usUsage     = 0x02,                   // mouse
        //.dwFlags     = RIDEV_INPUTSINK       // receive even when not focused
        //            | RIDEV_NOLEGACY,      // no WM_MOUSEMOVE fallback
        .dwFlags = 0,
        .hwndTarget  = global_w32_window.handle,
    };

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        // failure!
    }
    // clamp the cursor to your client rect if you want absolute confinement:
    #endif                        

    ShowCursor(0);

    OpenGL* opengl = arena_push_size(&g_arena, OpenGL, 1);
    opengl_init(opengl, global_w32_window);


    // ui state init
    UIState *ui_state = arena_push_size(&g_arena, UIState, 1);
    {
        ui_state->ortho_proj_mat = glm::ortho(0.0f, f32(SRC_WIDTH), f32(SRC_HEIGHT), 0.0f, -1.0f, 1.0f);
        // fonts
        ui_state->font_info = font_info;
    }



    init_ui(opengl, ui_state);
    #if 0
    {
        /* TODO The biggest problem here is do I recreate the render group every frame or not
            Because why would I do this if I can instead save the points in each model? Which is why they do

            I'm not sure how Casey and Allen code makes sense in this situation and how do they decide what or what not to render. And if they dont want to render something
            they must not include it. This render group thing only makes sense for things that are constantly changing, maybe for 2D is alright, like an immediate mode system.

            But... for persistent objects? I don't think so
        */
        /*
        const glm::vec3 T0v = { -3.0f, 1.0f, -4.5f};
        const glm::vec3 T1v = {-2.0f,  1.0f, -4.5f};
        const glm::vec3 T2v = {-1.0f,  1.0f, -4.5f};
        push_quad(render_group, T0v, glm::vec3(0.5f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        push_quad(render_group, T1v, glm::vec3(0.5f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        push_quad(render_group, T2v, glm::vec3(0.5f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        */

        glm::vec2 a_uv0 = glm::vec2(0.0084, 0.2256);
        glm::vec2 a_uv1 = glm::vec2(0.0462, 0.2256);
        glm::vec2 a_uv3 = glm::vec2(0.0084, 0.1429);
        glm::vec2 a_uv2 = glm::vec2(0.0462, 0.1429);

        #if 0
        // for fonts:
        #if 0
        int w = glyph.bitmap.width;
        int h = glyph.bitmap.height;
        #else
        TestPackerResult jaja = test_packer();
        int w = jaja.width;
        int h = jaja.height;
        #endif

        const glm::vec3 T0v = {100.0f,  500.0f, 0.0f};
        const glm::vec3 T1v = {100.0f + w,  500.0f, 0.0f};
        const glm::vec3 T2v = {100.0f + w,  500.0f - h, 0.0f};
        const glm::vec3 T3v = {100.0f,  500.0f - h, 0.0f};
        glm::vec2 uv0 = glm::vec2(0, 1);
        glm::vec2 uv1 = glm::vec2(1, 1);
        glm::vec2 uv2 = glm::vec2(1, 0);
        glm::vec2 uv3 = glm::vec2(0, 0);
        const glm::vec3 rect_points[4] = {T0v, T1v, T2v, T3v};
        push_rect(ui_render_group, rect_points, uv0, uv1, uv2, uv3);
        #endif


        glm::vec3 tri_points[3] = {glm::vec3(500.0f, 500.0f, 0.0f), glm::vec3(600.0f, 500.0f, 0.0f), glm::vec3(450.0f, 300.0f, 0.0f)};
        u16 tri_indices[3] = {0, 1, 2};
        push_triangle(ui_render_group, tri_points, tri_indices);
    }
    #endif

    // uniform init
    size_t uniform_matrix_buffer_size = 3 * sizeof(glm::mat4);
    mUniformBuffer.init(opengl, uniform_matrix_buffer_size);

    // ssbo init!
    mShaderBoneMatrixBuffer.init(opengl, 256);
    mShaderModelRootMatrixBuffer.init(opengl, 256);
    mShaderTRSMatrixBuffer.init(opengl, 256);
    mNodeTransformBuffer.init(opengl, 256);

    // TODO see todos
    Shader skinning_shader{};
    shader_init(&skinning_shader, opengl, str8("skel_shader-2.vs.glsl"), str8("6.multiple_lights.fs.glsl"));

    shader_init(&outline_shader, opengl, str8("skel_shader-2.vs.glsl"), str8("outline.fs.glsl"));

    {
        // test framebuffer!
        test_fb.pid = create_program(opengl, str8("tests/basic.vs.glsl"), str8("tests/basic.fs.glsl"));

        float vertices_rot[] = 
        {
            -0.5f, 0.7f, 	0.0f,  0.0f,    // Bottom-left
            0.5f, 0.7f, 	1.0f,  0.0f, 	// bottom-right         
            0.5f,  1.0f, 	1.0f,  1.0f, 	// top-right 

            0.5f,  1.0f, 	1.0f,  1.0f, 	// top-right
            -0.5f,  1.0f, 	0.0f,  1.0f, 	// top-left
            -0.5f,  0.7f, 	0.0f,  0.0f, 	// bottom-left
        };
        float vertices[] = 
        {
            -1.0f, -1.0f, 	0.0f,  0.0f,    // Bottom-left
            1.0f, -1.0f, 	1.0f,  0.0f, 	// bottom-right         
            1.0f,  1.0f, 	1.0f,  1.0f, 	// top-right 

            1.0f,  1.0f, 	1.0f,  1.0f, 	// top-right
            -1.0f,  1.0f, 	0.0f,  1.0f, 	// top-left
            -1.0f,  -1.0f, 	0.0f,  0.0f, 	// bottom-left
        };

        // this is part of `fb_fuckery`
        opengl->glGenVertexArrays(1, &test_fb.vao);
        opengl->glGenBuffers(1, &test_fb.vbo);
        opengl->glGenBuffers(1, &test_fb.ebo);

        opengl->glBindBuffer(GL_ARRAY_BUFFER, test_fb.vbo);
        opengl->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        opengl->glBindVertexArray(test_fb.vao);

        // position attribute
        opengl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        opengl->glEnableVertexAttribArray(0);

        // texture attribute
        opengl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        opengl->glEnableVertexAttribArray(1);

        opengl->glBindVertexArray(0);

        // rear view mirror!
        opengl->glGenVertexArrays(1, &test_fb.vao_rot);
        opengl->glGenBuffers(1, &test_fb.vbo_rot);
        opengl->glGenBuffers(1, &test_fb.ebo_rot);

        opengl->glBindBuffer(GL_ARRAY_BUFFER, test_fb.vbo_rot);
        opengl->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices_rot, GL_STATIC_DRAW);

        opengl->glBindVertexArray(test_fb.vao_rot);

        // position attribute
        opengl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        opengl->glEnableVertexAttribArray(0);

        // texture attribute
        opengl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        opengl->glEnableVertexAttribArray(1);

        opengl->glBindVertexArray(0);
    }
    
    // placeholder state init
    PlaceholderState *placeholder_state = arena_push_size(&g_arena, PlaceholderState, 1);
    placeholder_state->nonskinned_pid = create_program(opengl, str8("assimp.vert"), str8("assimp.frag"));
    placeholder_state->skinned_pid = create_program(opengl, str8("assimp_skinning.vert"), str8("assimp_skinning.frag"));
    placeholder_state->transform_compute_shader_pid = create_program(opengl, str8(0, 0), str8(0, 0), str8("assimp_instance_transform.comp"));
    placeholder_state->matrix_compute_shader_pid = create_program(opengl, str8(0, 0), str8(0, 0), str8("assimp_instance_matrix_mult.comp"));
    placeholder_state->persp_proj = glm::perspective(glm::radians(curr_camera->zoom), (float)SRC_WIDTH / (float)SRC_HEIGHT, 0.1f, 10000.0f);
    placeholder_state->instances = new InstancesHolder();
    {
        // basically the same as init_ui but i havent decided what the fuck to do yet!
        u32 static_mesh_pid = create_program(opengl, str8("static_mesh.vert.glsl"), str8("static_mesh.frag.glsl"));
        if(static_mesh_pid)
        {
            placeholder_state->static_mesh_pid = static_mesh_pid;

            opengl->glGenVertexArrays(1, &placeholder_state->vao);
            opengl->glGenBuffers(1, &placeholder_state->vbo);
            opengl->glGenBuffers(1, &placeholder_state->ebo);

            opengl->glBindVertexArray(placeholder_state->vao);

            opengl->glBindBuffer(GL_ARRAY_BUFFER, placeholder_state->vbo);
            opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, placeholder_state->ebo);

            // position attribute
            opengl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TextureQuadVertex), (void*)OffsetOf(TextureQuadVertex, p));
            // uv attribute
            opengl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextureQuadVertex), (void*)OffsetOf(TextureQuadVertex, uv));
            // color attribute
            opengl->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(TextureQuadVertex), (void*)OffsetOf(TextureQuadVertex, c));

            opengl->glEnableVertexAttribArray(0);
            opengl->glEnableVertexAttribArray(1);
            opengl->glEnableVertexAttribArray(2);

            placeholder_state->proj_loc = opengl->glGetUniformLocation(placeholder_state->static_mesh_pid, "proj");
            placeholder_state->view_loc= opengl->glGetUniformLocation(placeholder_state->static_mesh_pid, "view");
            placeholder_state->texture_sampler_loc = opengl->glGetUniformLocation(placeholder_state->static_mesh_pid, "texture_sampler");

            opengl->glBindVertexArray(0);

        }
    }


    #if notebook
    const char *model_filepath = "C:/Users/marcos/Desktop/Mastering-Cpp-Game-Animation-Programming/chapter01/01_opengl_assimp/assets/woman/Woman.gltf";
    #else
    const char *model_filepath = "E:/Mastering-Cpp-Game-Animation-Programming/chapter01/01_opengl_assimp/assets/woman/Woman.gltf";
    #endif
    add_instances(opengl, placeholder_state->instances, model_filepath, 10);

    f32 pre_transformed_quad[] = 
    {
		-0.5f, -0.5f,  0.5f, // Bottom-left
		 0.5f, -0.5f,  0.5f, // top-rightopengl, 
		 0.5f,  0.5f,  0.5f, // bottom-right         
		 0.5f,  0.5f,  0.5f, // top-right
		-0.5f,  0.5f,  0.5f, // bottom-left
		-0.5f, -0.5f,  0.5f // top-left
    };
    struct v3
    {
        f32 x, y, z;
    };

    const v3 T0 = { -3.0f, 1.0f, -5.0f };
    const v3 T1 = {-2.0f,  1.0f, -5.0f};
    const v3 T2 = {-1.0f,  1.0f, -5.0f};

    f32 transformed_quad[] = 
    {
        // MeshBox 0
        -0.5f + T0.x,  -0.5f + T0.y,  0.5f + T0.z,  1.0f, 0.0f, 1.0f, 1.0f,
        0.5f + T0.x,  -0.5f + T0.y,  0.5f + T0.z,  1.0f, 1.0f, 1.0f, 1.0f, 
        0.5f + T0.x,   0.5f + T0.y,  0.5f + T0.z,  1.0f, 0.0f, 1.0f, 1.0f, 
        0.5f + T0.x,   0.5f + T0.y,  0.5f + T0.z,  1.0f, 0.0f, 0.0f, 1.0f, 
        -0.5f + T0.x,   0.5f + T0.y,  0.5f + T0.z, 0.0f, 1.0f, 0.0f, 1.0f, 
        -0.5f + T0.x,  -0.5f + T0.y,  0.5f + T0.z, 0.0f, 0.0f, 1.0f, 1.0f, 

        // MeshBox 1
        -0.5f + T1.x,  -0.5f + T1.y,  0.5f + T1.z, 1.0f, 0.5f, 0.3f, 1.0f, 
        0.5f + T1.x,  -0.5f + T1.y,  0.5f + T1.z,  1.0f, 0.5f, 0.3f, 1.0f, 
        0.5f + T1.x,   0.5f + T1.y,  0.5f + T1.z,  1.0f, 0.5f, 0.3f, 1.0f, 
        0.5f + T1.x,   0.5f + T1.y,  0.5f + T1.z,  1.0f, 0.5f, 0.3f, 1.0f, 
        -0.5f + T1.x,   0.5f + T1.y,  0.5f + T1.z, 1.0f, 0.5f, 0.3f, 1.0f, 
        -0.5f + T1.x,  -0.5f + T1.y,  0.5f + T1.z, 1.0f, 0.5f, 0.3f, 1.0f, 

        // MeshBox 2
        -0.5f + T2.x,  -0.5f + T2.y,  0.5f + T2.z, 0.3f, 1.0f, 1.0f, 1.0f, 
        0.5f + T2.x,  -0.5f + T2.y,  0.5f + T2.z,  0.3f, 1.0f, 1.0f, 1.0f, 
        0.5f + T2.x,   0.5f + T2.y,  0.5f + T2.z,  0.3f, 1.0f, 1.0f, 1.0f, 
        0.5f + T2.x,   0.5f + T2.y,  0.5f + T2.z,  0.3f, 1.0f, 1.0f, 1.0f, 
        -0.5f + T2.x,   0.5f + T2.y,  0.5f + T2.z, 0.3f, 1.0f, 1.0f, 1.0f, 
        -0.5f + T2.x,  -0.5f + T2.y,  0.5f + T2.z,  0.3f, 1.0f, 1.0f, 1.0f,
    };



	float vertices[] = {
		// Back face
		-0.5f, -0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			0.0f, 0.0f, // Bottom-left
		 0.5f,  0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			1.0f, 1.0f, // top-rightopengl, 
		 0.5f, -0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			1.0f, 0.0f, // bottom-right         
		 0.5f,  0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			1.0f, 1.0f, // top-right
		-0.5f, -0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			0.0f, 0.0f, // bottom-left
		-0.5f,  0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			0.0f, 1.0f, // top-left
		// Front face        
		-0.5f, -0.5f,  0.5f,	0.0f,  0.0f,  1.0f,			0.0f, 0.0f, // bottom-left
		 0.5f, -0.5f,  0.5f,	0.0f,  0.0f,  1.0f,			1.0f, 0.0f, // bottom-right
		 0.5f,  0.5f,  0.5f,	0.0f,  0.0f,  1.0f,			1.0f, 1.0f, // top-right
		 0.5f,  0.5f,  0.5f,	0.0f,  0.0f,  1.0f,			1.0f, 1.0f, // top-right
		-0.5f,  0.5f,  0.5f,	0.0f,  0.0f,  1.0f,			0.0f, 1.0f, // top-left
		-0.5f, -0.5f,  0.5f,	0.0f,  0.0f,  1.0f,			0.0f, 0.0f, // bottom-left
		// Left face         
		-0.5f,  0.5f,  0.5f,	-1.0f,  0.0f,  0.0f,			1.0f, 0.0f, // top-right
		-0.5f,  0.5f, -0.5f,	-1.0f,  0.0f,  0.0f,			1.0f, 1.0f, // top-left
		-0.5f, -0.5f, -0.5f,	-1.0f,  0.0f,  0.0f,			0.0f, 1.0f, // bottom-left
		-0.5f, -0.5f, -0.5f,	-1.0f,  0.0f,  0.0f,			0.0f, 1.0f, // bottom-left
		-0.5f, -0.5f,  0.5f,	-1.0f,  0.0f,  0.0f,			0.0f, 0.0f, // bottom-right
		-0.5f,  0.5f,  0.5f,	-1.0f,  0.0f,  0.0f,			1.0f, 0.0f, // top-right
		// Right face        
		 0.5f,  0.5f,  0.5f,	1.0f,  0.0f,  0.0f,			1.0f, 0.0f, // top-left
		 0.5f, -0.5f, -0.5f,	1.0f,  0.0f,  0.0f,			0.0f, 1.0f, // bottom-right
		 0.5f,  0.5f, -0.5f,	1.0f,  0.0f,  0.0f,			1.0f, 1.0f, // top-right         
		 0.5f, -0.5f, -0.5f,	1.0f,  0.0f,  0.0f,			0.0f, 1.0f, // bottom-right
		 0.5f,  0.5f,  0.5f,	1.0f,  0.0f,  0.0f,			1.0f, 0.0f, // top-left
		 0.5f, -0.5f,  0.5f,	1.0f,  0.0f,  0.0f,			0.0f, 0.0f, // bottom-left     
		 // Bottom face       
		 -0.5f, -0.5f, -0.5f,	0.0f, -1.0f,  0.0f,			0.0f, 1.0f, // top-right
		  0.5f, -0.5f, -0.5f,	0.0f, -1.0f,  0.0f,			1.0f, 1.0f, // top-left
		  0.5f, -0.5f,  0.5f,	0.0f, -1.0f,  0.0f,			1.0f, 0.0f, // bottom-left
		  0.5f, -0.5f,  0.5f,	0.0f, -1.0f,  0.0f,			1.0f, 0.0f, // bottom-left
		 -0.5f, -0.5f,  0.5f,	0.0f, -1.0f,  0.0f,			0.0f, 0.0f, // bottom-right
		 -0.5f, -0.5f, -0.5f,	0.0f, -1.0f,  0.0f,			0.0f, 1.0f, // top-right
		 // Top face          
		 -0.5f,  0.5f, -0.5f,	0.0f,  1.0f,  0.0f,			0.0f, 1.0f, // top-left
		  0.5f,  0.5f,  0.5f,	0.0f,  1.0f,  0.0f,			1.0f, 0.0f, // bottom-right
		  0.5f,  0.5f, -0.5f,	0.0f,  1.0f,  0.0f,			1.0f, 1.0f, // top-right     
		  0.5f,  0.5f,  0.5f,	0.0f,  1.0f,  0.0f,			1.0f, 0.0f, // bottom-right
		 -0.5f,  0.5f, -0.5f,	0.0f,  1.0f,  0.0f,			0.0f, 1.0f, // top-left
		 -0.5f,  0.5f,  0.5f,	0.0f,  1.0f,  0.0f,			0.0f, 0.0f  // bottom-left        
	};
    unsigned int VBO, cubeVAO;
	opengl->glGenVertexArrays(1, &cubeVAO);
	opengl->glGenBuffers(1, &VBO);

	opengl->glBindBuffer(GL_ARRAY_BUFFER, VBO);
	opengl->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	opengl->glBindVertexArray(cubeVAO);

	// position attribute
	opengl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	opengl->glEnableVertexAttribArray(0);

	// normal attribute
	opengl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	opengl->glEnableVertexAttribArray(1);

	// texture attribute
	opengl->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	opengl->glEnableVertexAttribArray(2);

	MeshBox floor_meshbox = MeshBox(Transform3D(glm::vec3(floor_pos.x, floor_pos.y, floor_pos.z), glm::vec3(floor_scale.x, floor_scale.y, floor_scale.z)));
    std::vector<MeshBox> boxes = 
	{
		///////
		MeshBox(Transform3D(glm::vec3(-3.0f,  1.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(-2.0f,  1.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  1.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  1.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f, 1.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(2.0f, 1.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(3.0f, 1.0f, -5.0f))),

		MeshBox(Transform3D(glm::vec3(-3.0f,  2.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(-2.0f,  2.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  2.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  2.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f, 2.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(2.0f, 2.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(3.0f, 2.0f, -5.0f))),

		MeshBox(Transform3D(glm::vec3(-3.0f,  3.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(-2.0f,  3.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  3.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  3.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f, 3.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(2.0f, 3.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(3.0f, 3.0f, -5.0f))),

		///////
		MeshBox(Transform3D(glm::vec3(0.0f,  13.0f,  0.0f))),
		MeshBox(Transform3D(glm::vec3(5.0f,  6.0f, -10.0f))),
		MeshBox(Transform3D(glm::vec3(-1.5f, 5.2f, -2.5f))),
		MeshBox(Transform3D(glm::vec3(-9.8f, 5.0f, -12.3f))),
		MeshBox(Transform3D(glm::vec3(2.4f, 3.4f, -3.5f))),
		MeshBox(Transform3D(glm::vec3(-8.7f,  6.0f, -7.5f))),
		MeshBox(Transform3D(glm::vec3(3.3f, 5.0f, -2.5f))),
		MeshBox(Transform3D(glm::vec3(1.5f,  5.0f, -2.5f))),
		MeshBox(Transform3D(glm::vec3(8.5f,  8.2f, -1.5f))),
		MeshBox(Transform3D(glm::vec3(-8.3f,  4.0f, -1.5f))),


		// a pile of boxes
		MeshBox(Transform3D(glm::vec3(0.0f,  20.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  22.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  24.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f,  26.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  27.0f, 0.0f))),


		MeshBox(Transform3D(glm::vec3(0.0f,  30.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  31.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  32.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f,  33.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  34.0f, 0.0f))),

		MeshBox(Transform3D(glm::vec3(0.0f,  30.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  31.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  32.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f,  33.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  34.0f, 0.0f))),

		MeshBox(Transform3D(glm::vec3(0.0f,  30.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  31.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  32.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f,  33.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  34.0f, 0.0f))),

		MeshBox(Transform3D(glm::vec3(0.0f,  30.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  31.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  32.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f,  33.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  34.0f, 0.0f))),

		MeshBox(Transform3D(glm::vec3(0.0f,  30.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  31.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  32.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f,  33.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  34.0f, 0.0f))),
	};

    PointLight point_lights[] = 
	{
		PointLight
		{
			.transform = Transform3D(glm::vec3(0.7f,  0.2f,  2.0f)),
			.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
			.diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f),
			.constant = 1.0f,
			.linear = 0.09f,
			.quadratic = 0.032f,
		},
		PointLight
		{
			.transform = Transform3D(glm::vec3(2.3f,  -3.3f,  -4.0f)),
			.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
			.diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f),
			.constant = 1.0f,
			.linear = 0.09f,
			.quadratic = 0.032f,
		},
		PointLight
		{
			.transform = Transform3D(glm::vec3(-4.0f,  2.0f,  -12.0f)),
			.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
			.diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f),
			.constant = 1.0f,
			.linear = 0.09f,
			.quadratic = 0.032f,
		},
		PointLight
		{
			.transform = Transform3D(glm::vec3(0.0f,  0.0f,  -3.0f)),
			.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
			.diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f),
			.constant = 1.0f,
			.linear = 0.09f,
			.quadratic = 0.032f,
		},
	};

	DirectionalLight directional_light
	{
		.direction = glm::vec3(-0.2f, -1.0f, -0.3f),
		.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
		.diffuse = glm::vec3(0.4f, 0.4f, 0.4f),
		.specular = glm::vec3(0.5f, 0.5f, 0.5f)
	};

	SpotLight spot_light
	{
		.ambient = glm::vec3(0.0f, 0.0f, 0.0f),
		.diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
		.specular = glm::vec3(1.0f, 1.0f, 1.0f),
		.constant = 1.0f,
		.linear = 0.09f,
		.quadratic = 0.032f,
		.cutOff = glm::cos(glm::radians(12.5f)),
		.outerCutOff = glm::cos(glm::radians(15.0f)),
	};

    shader_use(skinning_shader);
    glm::vec3 model_color = glm::vec3(1.0f, 0.5f, 0.31f);
    shader_set_vec3(skinning_shader, "objectColor", model_color.r, model_color.g, model_color.b);
    shader_set_int(skinning_shader, "material.diffuse", 0);
    shader_set_int(skinning_shader, "material.specular", 1);
    shader_set_float(skinning_shader, "material.shininess", model_material_shininess);


    // directional_light
    shader_set_vec3(skinning_shader, "dirLight.direction", directional_light.direction);
    shader_set_vec3(skinning_shader, "dirLight.ambient", directional_light.ambient);
    shader_set_vec3(skinning_shader, "dirLight.diffuse", directional_light.diffuse);
    shader_set_vec3(skinning_shader, "dirLight.specular", directional_light.specular);

    // point_lights
    int n = sizeof(point_lights) / sizeof(point_lights[0]);
    TempArena per_frame = temp_begin(&g_transient_arena);
    for (int i = 0; i < n; i++) {
        aim_profiler_time_block("pointLights creation");
        {
            Str8 prefix = str8_fmt(per_frame.arena, "pointLights[%d]", i);
            shader_set_vec3(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".position")), point_lights[i].transform.pos);
            shader_set_vec3(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".ambient")), point_lights[i].ambient);
            shader_set_vec3(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".diffuse")), point_lights[i].diffuse);
            shader_set_vec3(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".specular")), point_lights[i].specular);
            shader_set_float(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".constant")), point_lights[i].constant);
            shader_set_float(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".linear")), point_lights[i].linear);
            shader_set_float(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".quadratic")), point_lights[i].quadratic);
        }
    }
    temp_end(per_frame);

    // spot_light
    shader_set_vec3(skinning_shader, "spotLight.ambient", spot_light.ambient);
    shader_set_vec3(skinning_shader, "spotLight.diffuse", spot_light.diffuse);
    shader_set_vec3(skinning_shader, "spotLight.specular", spot_light.specular);
    shader_set_float(skinning_shader, "spotLight.constant", spot_light.constant);
    shader_set_float(skinning_shader, "spotLight.linear", spot_light.linear);
    shader_set_float(skinning_shader, "spotLight.quadratic", spot_light.quadratic);
    shader_set_float(skinning_shader, "spotLight.cutOff", spot_light.cutOff);
    shader_set_float(skinning_shader, "spotLight.outerCutOff", spot_light.outerCutOff);
    shader_set_mat4(skinning_shader, "projection", placeholder_state->persp_proj);

    //while (!glfwWindowShouldClose(window)) 
    LONGLONG frequency = aim_timer_get_os_freq();
    LONGLONG last_frame = 0;
    global_w32_window.is_running = true;
    glViewport(0, 0, SRC_WIDTH, SRC_HEIGHT);

    HDC hdc = GetDC(global_w32_window.handle);
    fb_fuckery(opengl, &test_fb, SRC_WIDTH, SRC_HEIGHT);
    while (global_w32_window.is_running)
	{

        LONGLONG now = aim_timer_get_os_time();
        LONGLONG dt_long = now - last_frame;
        last_frame = now;
        f32 dt = aim_timer_ticks_to_sec(dt_long, frequency);
        f32 dt_ms = aim_timer_ticks_to_ms(dt_long, frequency);
        TempArena per_frame = temp_begin(&g_transient_arena);

        // begin render group
        UIRenderGroup *ui_render_group = arena_push_size(per_frame.arena, UIRenderGroup, 1);
        ui_state->render_group = ui_render_group;
        ui_render_group->vertex_array = arena_push_size(per_frame.arena, TextureQuadVertex, max_vertex_per_batch);
        ui_render_group->index_array = arena_push_size(per_frame.arena, u16, max_index_per_batch);
        ui_render_group->vertex_count = 0;
        ui_render_group->index_count  = 0;

        UIRenderGroup *placeholder_render_group = arena_push_size(per_frame.arena, UIRenderGroup, 1);
        placeholder_state->static_mesh_render_group = placeholder_render_group;
        placeholder_render_group->vertex_array = arena_push_size(per_frame.arena, TextureQuadVertex, max_vertex_per_batch);
        placeholder_render_group->index_array = arena_push_size(per_frame.arena, u16, max_index_per_batch);
        placeholder_render_group->vertex_count = 0;
        placeholder_render_group->index_count  = 0;

        global_input.dt = dt;
        {
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "Frame time: %.4fms";
            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, dt_ms);
            push_text(ui_state, at, 25, 75);
        }
        {
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "Cam pos: %.4f, %.4f, %.4f";
            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, curr_camera->position.x, curr_camera->position.y, curr_camera->position.z);
            push_text(ui_state, at, 25, 100);
        }
        {
            char buf[100];
            char *at = buf;
            char *end = buf + sizeof(buf);
            const char* c  = "Mouse screen coordinates: %.2f, %.2f";
            
            f32 mouse_p_x = global_input.curr_mouse_state.x;
            f32 mouse_p_y = global_input.curr_mouse_state.y;
            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), c, mouse_p_x, mouse_p_y);
            push_text(ui_state, at, 25, 125);
        }


        {
            glm::vec3 from = glm::vec3(0.0f, 0.0f, 0.0f);
            glm::vec3 to = glm::vec3(10.0f, 2.0f, 10.0f);
            push_line(placeholder_state->static_mesh_render_group, from, to);
        }

        win32_process_pending_msgs();
        if (input_is_key_pressed(&global_input, Keys_W))
            curr_camera->process_keyboard(FORWARD, dt);
        if (input_is_key_pressed(&global_input, Keys_S))
            curr_camera->process_keyboard(BACKWARD, dt);
        if (input_is_key_pressed(&global_input, Keys_A))
            curr_camera->process_keyboard(LEFT, dt);
        if (input_is_key_pressed(&global_input, Keys_D))
            curr_camera->process_keyboard(RIGHT, dt);
        if (input_is_key_pressed(&global_input, Keys_Space))
			curr_camera->process_keyboard(UP, dt);
        if (input_is_key_pressed(&global_input, Keys_Control))
            curr_camera->process_keyboard(DOWN, dt);

        {
            f32 xx = global_input.dx;
            f32 yy = global_input.dy;
            curr_camera->process_mouse_movement(xx, yy);
        }

        update_and_render(placeholder_state);

        //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
        opengl_render(opengl, curr_camera, cubeVAO, skinning_shader, floor_meshbox, boxes, placeholder_state, ui_state, dt);
        //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);

        input_update(&global_input);
        SwapBuffers(hdc);
        temp_end(per_frame);
    }
    ReleaseDC(global_w32_window.handle, hdc);
	aim_profiler_end();
	aim_profiler_print();
}