#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <string>

#include "samples/opengl_bindings.cpp"
// thirdparty

/* TODO primero ver de sacar lo que esta en shader, capaz es el causante del internal
    y no glm. Seria ideal asi no tengo que renombrar todo ahora, y de paso tengo
    que dejar de usar la poronga de string igual asi que...

    primero migrar glfw, despues shader, despues chequear el internal
*/
#define internal_linkage static
//#define internal static
#define global_variable static
#define local_persist static
#define RAW_INPUT 1

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;
typedef uint32_t b32;
typedef u32 b32;

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#if 0
// assimp
#include "AssimpLoader.h"

// jolt
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
// vehicle physics
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
// debug renderer
#include <Jolt/Renderer/DebugRenderer.h>
#include "physics/jolt_debug_renderer.h"
#include "physics/physics_system.h"

#include "physics/jolt_debug_renderer.cpp"
#include "physics/physics_system.cpp"
#endif


// TODO fix assert collision with windows.h or something else!

#define gui_assert(expr, fmt, ...) do {                                       \
    if (!(expr)) {                                                            \
        char _msg_buf[1024];                                                  \
                snprintf(_msg_buf, sizeof(_msg_buf),                                        \
            "Assertion failed!\n\n"                                                 \
            "File: %s\n"                                                            \
            "Line: %d\n"                                                            \
            "Condition: %s\n\n"                                                     \
            fmt,                                                                    \
            __FILE__, __LINE__, #expr, __VA_ARGS__);                                \
        MessageBoxA(0, _msg_buf, "Assertion Failed", MB_OK | MB_ICONERROR);   \
        __debugbreak();                                                       \
    }                                                                         \
} while (0)

//#include "base/base_core.h"
#include "base/base_arena.h"
#include "base/base_string.h"
#include "os/os_core.h"
#include "base/base_arena.cpp"
#include "base/base_string.cpp"
#include "os/os_core.cpp"
#include "input/input.h"
typedef Input GameInput;
#include "camera.h"
//TODO: get the timer
#include "aim_timer.h"


#include "camera.cpp"
#include "aim_timer.cpp"

#include "components.h"
using namespace aim::Components;

// TODO fix
//#include "logger/logger.h"
//#include "logger/logger.cpp"

#define OpenGLSetFunction(name) (opengl->name) = OpenGLGetFunction(name);
#define OpenGLDeclareMemberFunction(name) OpenGLType_##name name

struct OpenGL
{
    i32 vsynch;
    // OpenGLType__wglCreateContextAttribsARB wglCreateContextAttribsARB
    OpenGLDeclareMemberFunction(wglCreateContextAttribsARB);
    OpenGLDeclareMemberFunction(wglGetExtensionsStringEXT);
    OpenGLDeclareMemberFunction(wglSwapIntervalEXT);
    OpenGLDeclareMemberFunction(wglGetSwapIntervalEXT);
    OpenGLDeclareMemberFunction(glGenVertexArrays);
    OpenGLDeclareMemberFunction(glBindVertexArray);
    OpenGLDeclareMemberFunction(glDeleteVertexArrays);

	OpenGLDeclareMemberFunction(glGenBuffers);
	OpenGLDeclareMemberFunction(glBindBuffer);
	OpenGLDeclareMemberFunction(glBufferData);
	OpenGLDeclareMemberFunction(glVertexAttribPointer);
	OpenGLDeclareMemberFunction(glEnableVertexAttribArray);

    OpenGLDeclareMemberFunction(glCreateShader);
    OpenGLDeclareMemberFunction(glCompileShader);
    OpenGLDeclareMemberFunction(glShaderSource);
    OpenGLDeclareMemberFunction(glCreateProgram);
    OpenGLDeclareMemberFunction(glAttachShader);
    OpenGLDeclareMemberFunction(glLinkProgram);
    OpenGLDeclareMemberFunction(glDeleteShader);
    OpenGLDeclareMemberFunction(glUseProgram);
    OpenGLDeclareMemberFunction(glGetShaderiv);
    OpenGLDeclareMemberFunction(glGetShaderInfoLog);
    OpenGLDeclareMemberFunction(glGetProgramiv);
    OpenGLDeclareMemberFunction(glGetProgramInfoLog);

    OpenGLDeclareMemberFunction(glUniform1i);
    OpenGLDeclareMemberFunction(glUniform1f);
    OpenGLDeclareMemberFunction(glUniform2fv);
    OpenGLDeclareMemberFunction(glUniform2f);
    OpenGLDeclareMemberFunction(glUniform3fv);
    OpenGLDeclareMemberFunction(glUniform3f);
    OpenGLDeclareMemberFunction(glUniform4fv);
    OpenGLDeclareMemberFunction(glUniform4f);
    OpenGLDeclareMemberFunction(glUniformMatrix2fv);
    OpenGLDeclareMemberFunction(glUniformMatrix3fv);
    OpenGLDeclareMemberFunction(glUniformMatrix4fv);
    OpenGLDeclareMemberFunction(glGetUniformLocation);
};

#include "shader.h"

///////// TODO cleanup ////////////
// global state
// window size
global_variable u32 SRC_WIDTH = 1280;
global_variable u32 SRC_HEIGHT = 720;

// TODO: Removed these
// used during mouse callback
global_variable float lastX = SRC_WIDTH / 2.0f;
global_variable float lastY = SRC_HEIGHT / 2.0f;
global_variable bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

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
glm::vec3 floor_scale(30.0f, 1.0f, 30.0f);
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

Camera& curr_camera = free_camera;
///////// TODO cleanup ////////////

struct MeshBox {
	Transform3D transform;
	//PhysicsBody body;

	MeshBox(Transform3D transform) {
		this->transform = transform;
	}

	// this is not the best api, its only used once when there is no body. Only done in creation
	// This api is more like a builder style api, its probably better to have it inside the constructor and that's it or use a builder for this shapes. which could be good.
	//void set_shape(JPH::Ref<JPH::Shape> shape) {
	//	this->body.shape = shape;
	//}

	//void update_body_shape(PhysicsSystem& physics_system) {
	//	JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(this->transform.scale.x / 2.0, this->transform.scale.y / 2.0, this->transform.scale.z / 2.0));
	//	floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
	//	JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
	//	JPH::Ref<JPH::Shape> floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()
	//	this->body.shape = floor_shape;
	//	physics_system.get_body_interface().SetShape(this->body.physics_body_id, this->body.shape, false, JPH::EActivation::DontActivate);

	//	glm::vec3 my_pos = this->transform.pos;
	//	JPH::Vec3 new_pos = JPH::Vec3(my_pos.x, my_pos.y, my_pos.z);
	//	physics_system.get_body_interface().SetPosition(this->body.physics_body_id, new_pos, JPH::EActivation::DontActivate);

	//	physics_system.get_body_interface().SetRotation(
	//		this->body.physics_body_id,
	//		JPH::Quat(this->transform.rot.x, this->transform.rot.y, this->transform.rot.z, this->transform.rot.w),
	//		JPH::EActivation::DontActivate
	//	);
	//}
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


#if 0
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// Note: width and height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	// im not updating the perspective camera correctly using the new properties of the window. It only works because the aspect ratio remains the same in my
	// normal usecase
}

void keyboard_callback(GLFWwindow* window, int key, int scan_code, int action, int mods) {
	u8 pressed = ((action == GLFW_PRESS || action == GLFW_REPEAT) == 1);
	game_input->keyboard_state.curr_state.keys[Keys(key)] = pressed;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;


	curr_camera.process_mouse_movement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	curr_camera.process_mouse_scroll(static_cast<float>(yoffset));
}
#endif
global_variable OS_Window global_w32_window;
global_variable u32 os_modifiers;
global_variable GameInput global_input;
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

                assert((xxPos == xPos && yyPos == yPos));
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

                gui_assert(size < 1000, "GetRawInputData surpassed 1000 bytes");
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
                    gui_assert(1 < 0, "MOUSE_MOVE_ABSOLUTE");
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
                assert('wtf');
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


int main() {
    global_w32_window = os_win32_open_window("opengl", SRC_WIDTH, SRC_HEIGHT, win32_main_callback, 1);

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

    OpenGL _opengl = {0};
    OpenGL *opengl = &_opengl;
    opengl->vsynch = -1;

    HDC hdc = GetDC(global_w32_window.handle);
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixel_format = ChoosePixelFormat(hdc, &pfd);
    if(pixel_format) {
        SetPixelFormat(hdc, pixel_format, &pfd);
    }else{
        __debugbreak();
    }

    HGLRC tempRC = wglCreateContext(hdc);
    if(wglMakeCurrent(hdc, tempRC))
    {
        // NOTE It seems that in order to get anything from `wglGetProcAddress`, `wglMakeCurrent` must have been called!
        //wglCreateContextAttribsARB = OpenGLGetFunction(wglCreateContextAttribsARB);
        OpenGLSetFunction(wglCreateContextAttribsARB)

        i32 attrib_list[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_FLAGS_ARB, 0,
            WGL_CONTEXT_PROFILE_MASK_ARB,
            WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0
        };
        HGLRC hglrc = opengl->wglCreateContextAttribsARB(hdc, 0, attrib_list);
        wglMakeCurrent(0, 0);
        wglDeleteContext(tempRC);
        wglMakeCurrent(hdc, hglrc);
    }
    OpenGLSetFunction(wglGetExtensionsStringEXT);
    {
        const char* extensions = opengl->wglGetExtensionsStringEXT();
        b32 swap_control_supported = false;
		const char* at = extensions;
        while(*at) 
        {
            while(*at == ' ') {
                at++;
            }

            const char* start = at;
            while(*at && *at != ' ') {
                at++; 
            }
            if (!*at) {
                break;
            }
            size_t count = at - start;
            Str8 extension = str8((char*)start, count);
            b32 res1 = str8_equals(extension, str8_lit("WGL_EXT_framebuffer_sRGB"));
            b32 res2 = str8_equals(extension, str8_lit("WGL_ARB_framebuffer_sRGB"));
            if(str8_equals(extension, str8_lit("WGL_EXT_swap_control")))
            {
                swap_control_supported = true;
            }

        }
        if(swap_control_supported) {
            OpenGLSetFunction(wglSwapIntervalEXT);
            OpenGLSetFunction(wglGetSwapIntervalEXT);
            if(opengl->wglSwapIntervalEXT(1)) 
            {
                opengl->vsynch = opengl->wglGetSwapIntervalEXT();
            }
        }
    }

    OpenGLSetFunction(glGenVertexArrays);
    OpenGLSetFunction(glBindVertexArray);
    OpenGLSetFunction(glDeleteVertexArrays);

	OpenGLSetFunction(glGenBuffers);
	OpenGLSetFunction(glBindBuffer);
	OpenGLSetFunction(glBufferData);
	OpenGLSetFunction(glVertexAttribPointer);
	OpenGLSetFunction(glEnableVertexAttribArray);

    OpenGLSetFunction(glCreateShader);
    OpenGLSetFunction(glCompileShader);
    OpenGLSetFunction(glShaderSource);
    OpenGLSetFunction(glCreateProgram);
    OpenGLSetFunction(glAttachShader);
    OpenGLSetFunction(glLinkProgram);
    OpenGLSetFunction(glDeleteShader);
    OpenGLSetFunction(glUseProgram);
    OpenGLSetFunction(glGetShaderiv);
    OpenGLSetFunction(glGetShaderInfoLog);
    OpenGLSetFunction(glGetProgramiv);
    OpenGLSetFunction(glGetProgramInfoLog);

    OpenGLSetFunction(glUniform1i);
    OpenGLSetFunction(glUniform1f);
    OpenGLSetFunction(glUniform2fv);
    OpenGLSetFunction(glUniform2f);
    OpenGLSetFunction(glUniform3fv);
    OpenGLSetFunction(glUniform3f);
    OpenGLSetFunction(glUniform4fv);
    OpenGLSetFunction(glUniform4f);
    OpenGLSetFunction(glUniformMatrix2fv);
    OpenGLSetFunction(glUniformMatrix3fv);
    OpenGLSetFunction(glUniformMatrix4fv);
    OpenGLSetFunction(glGetUniformLocation);

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// physics-init
	#if 0
    JPH::RegisterDefaultAllocator();
	PhysicsSystem physics_system{};
	JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(1.5f), JPH::RVec3(0.0_r, 15.0_r, 0.0_r), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
	sphere_settings.mMassPropertiesOverride = JPH::MassProperties{ .mMass = 40.0f };
	Transform3D sphere_transform = Transform3D(glm::vec3(0.0, 15.0, 0.0));
	JPH::BodyID my_sphere = physics_system.create_body(&sphere_transform, new JPH::SphereShape(1.5f), false);
	physics_system.get_body_interface().GetShape(my_sphere);
	physics_system.get_body_interface().SetLinearVelocity(my_sphere, JPH::Vec3(0.0f, -2.0f, 0.0f));
	physics_system.get_body_interface().SetRestitution(my_sphere, 0.5f);

	JPH::RefConst<JPH::Shape> mStandingShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * 1.0 + 1.0, 0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f * 1.0, 1.0)).Create().Get();

	JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
	settings->mMaxStrength = 5000.0f;
	settings->mMass = 70.0f;
	settings->mShape = mStandingShape;
	JPH::Ref<JPH::CharacterVirtual> mCharacter = new JPH::CharacterVirtual(settings, JPH::Vec3::sZero(), JPH::Quat::sIdentity(), 0, &physics_system.inner_physics_system);
	#endif

    // rendering
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(curr_camera.zoom), (float)SRC_WIDTH / (float)SRC_HEIGHT, 0.1f, 10000.0f);

    // shaders
    //Shader skinning_shader("skel_shader-2.vs.glsl", "6.multiple_lights.fs.glsl", opengl);
    Shader skinning_shader = {};
    //shader_init(&skinning_shader, opengl, str8("skel_shader-2.vs.glsl"), str8("6.multiple_lights.fs.glsl"));
    shader_init(&skinning_shader, opengl, "skel_shader-2.vs.glsl", "6.multiple_lights.fs.glsl");
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

    //while (!glfwWindowShouldClose(window)) 
    LONGLONG frequency = aim_timer_get_os_freq();
    LONGLONG last_frame = 0;
    global_w32_window.is_running = true;
    glViewport(0, 0, SRC_WIDTH, SRC_HEIGHT);
    while (global_w32_window.is_running)
	{
        LONGLONG now = aim_timer_get_os_time();
        LONGLONG dt_long = now - last_frame;
        last_frame = now;
        f32 dt = aim_timer_ticks_to_sec(dt_long, frequency);
        global_input.dt = dt;

        win32_process_pending_msgs();
        if (input_is_key_pressed(&global_input, Keys_W))
            curr_camera.process_keyboard(FORWARD, dt);
        if (input_is_key_pressed(&global_input, Keys_S))
            curr_camera.process_keyboard(BACKWARD, dt);
        if (input_is_key_pressed(&global_input, Keys_A))
            curr_camera.process_keyboard(LEFT, dt);
        if (input_is_key_pressed(&global_input, Keys_D))
            curr_camera.process_keyboard(RIGHT, dt);
        if (input_is_key_pressed(&global_input, Keys_Space))
			curr_camera.process_keyboard(UP, dt);
        if (input_is_key_pressed(&global_input, Keys_Control))
            curr_camera.process_keyboard(DOWN, dt);

        
        //if(global_input.curr_mouse_state.x - )
        {

            f32 xx = global_input.dx;
            f32 yy = global_input.dy;
            curr_camera.process_mouse_movement(xx, yy);
        }
        //global_input.curr_mouse_state.y = yPos;

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(1.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glm::mat4 view = curr_camera.GetViewMatrix();

        shader_use(skinning_shader);
		shader_set_mat4(skinning_shader, "nodeMatrix", glm::mat4(1.0f));
        glm::vec3 model_color = glm::vec3(1.0f, 0.5f, 0.31f);
		shader_set_vec3(skinning_shader, "objectColor", model_color.r, model_color.g, model_color.b);
		shader_set_int(skinning_shader, "material.diffuse", 0);
		shader_set_int(skinning_shader, "material.specular", 1);
		shader_set_float(skinning_shader, "material.shininess", model_material_shininess);

        shader_set_vec3(skinning_shader, "viewPos", curr_camera.position);
        shader_set_vec3(skinning_shader, "spotLight.position", curr_camera.position);
        shader_set_vec3(skinning_shader, "spotLight.direction", curr_camera.forward);

		// directional_light
		shader_set_vec3(skinning_shader, "dirLight.direction", directional_light.direction);
		shader_set_vec3(skinning_shader, "dirLight.ambient", directional_light.ambient);
		shader_set_vec3(skinning_shader, "dirLight.diffuse", directional_light.diffuse);
		shader_set_vec3(skinning_shader, "dirLight.specular", directional_light.specular);

		// point_lights
		int n = sizeof(point_lights) / sizeof(point_lights[0]);
		for (int i = 0; i < n; i++) {
            //Str8 prefix = str8_fmt(&per_frame_arena, "pointLights[%d]", i);
            
			//shader_set_vec3(skinning_shader, str8_concat(prefix, str8(".position")), point_lights[i].transform.pos);
			//shader_set_vec3(skinning_shader, str8_concat(prefix, str8(".ambient")), point_lights[i].ambient);
			//shader_set_vec3(skinning_shader, str8_concat(prefix, str8(".diffuse")), point_lights[i].diffuse);
			//shader_set_vec3(skinning_shader, str8_concat(prefix, str8(".specular")), point_lights[i].specular);
			//shader_set_float(skinning_shader, str8_concat(prefix, str8(".constant")), point_lights[i].constant);
			//shader_set_float(skinning_shader, str8_concat(prefix, str8(".linear")), point_lights[i].linear);
			//shader_set_float(skinning_shader, str8_concat(prefix, str8(".quadratic")), point_lights[i].quadratic);
            

            #if 0
            // NOTE using _sprintf_s here works because it appends a '\0' after processing. If not I would have to advance at by 
            // the total written, place a '\0' and come back
            // 
            // Remark: "The _snprintf_s function formats and stores count or fewer characters in buffer and appends a terminating NULL."
            // Source: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/snprintf-s-snprintf-s-l-snwprintf-s-snwprintf-s-l?view=msvc-170#remarks
            char prefix[300];
            char *end = prefix + sizeof(prefix);
            char *at = prefix;
            at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "pointLights[%d]", i);

            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".position");
			shader_set_vec3(skinning_shader, prefix, point_lights[i].transform.pos);

            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".ambient");
			shader_set_vec3(skinning_shader, prefix, point_lights[i].ambient);

            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".diffuse");
			shader_set_vec3(skinning_shader, prefix, point_lights[i].diffuse);

            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".specular");
			shader_set_vec3(skinning_shader, prefix, point_lights[i].specular);

            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".constant");
			shader_set_float(skinning_shader, prefix, point_lights[i].constant);

            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".linear");
			shader_set_float(skinning_shader, prefix, point_lights[i].linear);

            _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".quadratic");
			shader_set_float(skinning_shader, prefix, point_lights[i].quadratic);
            #endif
		}

		// spot_light
		shader_set_vec3(skinning_shader, "spotLight.ambient", spot_light.ambient);
		shader_set_vec3(skinning_shader, "spotLight.diffuse", spot_light.diffuse);
		shader_set_vec3(skinning_shader, "spotLight.specular", spot_light.specular);
		shader_set_float(skinning_shader, "spotLight.constant", spot_light.constant);
		shader_set_float(skinning_shader, "spotLight.linear", spot_light.linear);
		shader_set_float(skinning_shader, "spotLight.quadratic", spot_light.quadratic);
		shader_set_float(skinning_shader, "spotLight.cutOff", spot_light.cutOff);
		shader_set_float(skinning_shader, "spotLight.outerCutOff", spot_light.outerCutOff);
		shader_set_mat4(skinning_shader, "projection", projection);
		shader_set_mat4(skinning_shader, "view", view);
        
		// bind diffuse map
        #if 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularMap);
		glBindVertexArray(cubeVAO);
        #endif

		// render floor, is just a plane
		glm::mat4 model_mat = glm::translate(glm::mat4(1.0f), floor_meshbox.transform.pos) *
			glm::mat4_cast(floor_meshbox.transform.rot) *
			glm::scale(glm::mat4(1.0f), floor_meshbox.transform.scale);

		shader_set_mat4(skinning_shader, "model", model_mat);
        // Esto va en OpenGL struct??
		glDrawArrays(GL_TRIANGLES, 0, 36);
        for (unsigned int i = 0; i < boxes.size(); i++)
        {
            // calculate the model matrix for each object and pass it to shader before drawing
            //glm::mat4 model = glm::mat4(1.0f);
            //model = glm::translate(model, boxes[i].transform.pos);
            //float angle = 20.0f * i;
            //model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f)); // take rotation out for testing

            // with rotations
            glm::mat4 model = glm::translate(glm::mat4(1.0f), boxes[i].transform.pos) *
                glm::mat4_cast(boxes[i].transform.rot) *
                glm::scale(glm::mat4(1.0f), boxes[i].transform.scale);
            shader_set_mat4(skinning_shader, "model", model);

            //shader_set_mat4("nodeMatrix", glm::mat4(1.0f));

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        input_update(&global_input);
        //game_input->keyboard_state.prev_state = game_input->keyboard_state.curr_state;
        //glfwPollEvents();
        //glfwSwapBuffers(window);
        SwapBuffers(hdc);
    }
}