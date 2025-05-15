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
#include "base/base_arena.cpp"
#include "base/base_string.cpp"
#include "os/os_core.cpp"
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

static Arena g_arena;
static Arena g_transient_arena;

#include "renderer.cpp"
#include "renderer/opengl_renderer.cpp"

#include "shader.h"

///////// TODO cleanup ////////////
// global state
// window size

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
    {  if (bufferData.empty()) {
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

    void uploadSsboData(OpenGL* opengl, std::vector<glm::mat4> bufferData, int bindingPoint) 
    {
        if (bufferData.empty()) {
            return;
        }

        size_t bufferSize = bufferData.size() * sizeof(glm::mat4);
        if (bufferSize > mBufferSize) {
            //Logger::log(1, "%s: resizing SSBO %i from %i to %i bytes\n", __FUNCTION__, mShaderStorageBuffer, mBufferSize, bufferSize);
            //cleanup();
            //init(bufferSize);
            abort();
        }

        opengl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
        opengl->glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, bufferData.data());
        opengl->glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingPoint, mShaderStorageBuffer, 0,
            bufferSize);
        opengl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
};

global_variable OS_Window global_w32_window;
global_variable u32 os_modifiers;
global_variable GameInput global_input;

global_variable UniformBuffer mUniformBuffer{};
/* for non-animated models */
global_variable std::vector<glm::mat4> mWorldPosMatrices{};
global_variable ShaderStorageBuffer mWorldPosBuffer{};

/* for animated models */
global_variable std::vector<glm::mat4> mModelBoneMatrices{};
global_variable ShaderStorageBuffer mShaderBoneMatrixBuffer{};

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

Camera* curr_camera = &free_camera;
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


struct Bone
{
    unsigned int mBoneId = 0;
    std::string mNodeName;
    glm::mat4 mOffsetMatrix = glm::mat4(1.0f);
};

struct OGLVertex {
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec4 color = glm::vec4(1.0f);
  glm::vec3 normal = glm::vec3(0.0f);
  glm::vec2 uv = glm::vec2(0.0f);
  glm::uvec4 boneNumber = glm::uvec4(0);
  glm::vec4 boneWeight = glm::vec4(0.0f);
};

struct OGLMesh {
  std::vector<OGLVertex> vertices{};
  std::vector<uint32_t> indices{};
  std::unordered_map<aiTextureType, std::string> textures{};
  bool usesPBRColors = false;
};

struct Texture
{
    GLuint mTexture = 0;
    int mTexWidth = 0;
    int mTexHeight = 0;
    int mNumberOfChannels = 0;
    std::string mTextureName;

    bool loadTexture(OpenGL* opengl, std::string textureFilename, bool flipImage = 1) 
    {
        mTextureName = textureFilename;

        stbi_set_flip_vertically_on_load(flipImage);
        /* always load as RGBA */
        unsigned char *textureData = stbi_load(textureFilename.c_str(), &mTexWidth, &mTexHeight, &mNumberOfChannels, STBI_rgb_alpha);

        if (!textureData) {
            // Logger::log(1, "%s error: could not load file '%s'\n", __FUNCTION__, mTextureName.c_str());
            stbi_image_free(textureData);
            return false;
        }

        glGenTextures(1, &mTexture);
        glBindTexture(GL_TEXTURE_2D, mTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

        opengl->glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(textureData);

        //Logger::log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, mTextureName.c_str(), mTexWidth, mTexHeight, mNumberOfChannels);
        return true;
    }

    bool loadTexture(OpenGL *opengl, std::string textureName, aiTexel* textureData, int width, int height, bool flipImage = true)
    {
        if (!textureData) {
            // Logger::log(1, "%s error: could not load texture '%s'\n", __FUNCTION__, textureName.c_str());
            return false;
        }

        // Logger::log(1, "%s: texture file '%s' has width %i and height %i\n", __FUNCTION__, textureName.c_str(), width, height);

        glGenTextures(1, &mTexture);
        glBindTexture(GL_TEXTURE_2D, mTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        /* allow to flip the image, similar to file loaded from disk */
        stbi_set_flip_vertically_on_load(flipImage);

        /* we use stbi to detect the in-memory format, but always request RGBA */
        unsigned char *data = nullptr;
        if (height == 0)   {
            data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(textureData), width, &mTexWidth, &mTexHeight, &mNumberOfChannels, STBI_rgb_alpha);
        }
        else   {
            data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(textureData), width * height, &mTexWidth, &mTexHeight, &mNumberOfChannels, STBI_rgb_alpha);
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);

        opengl->glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Logger::log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, textureName.c_str(), mTexWidth, mTexHeight, mNumberOfChannels);
        return true;
    }

    void bind() 
    {
        glBindTexture(GL_TEXTURE_2D, mTexture);
    }

    void unbind() 
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

struct Mesh
{
    std::string mMeshName;
    unsigned int mTriangleCount = 0;
    unsigned int mVertexCount = 0;

    glm::vec4 mBaseColor = glm::vec4(1.0f);

    OGLMesh mMesh{};
    //std::unordered_map<std::string, std::shared_ptr<Texture>> mTextures{};
    std::unordered_map<std::string, Texture*> mTextures{};

    //std::vector<Bone*> mBoneList{};


};

struct Node
{
    std::string nodeName;
    // Node *first;
    // Node *last;
    std::vector<Node*> mChildNodes{};

    Node *mParentNode;

    glm::vec3 mTranslation = glm::vec3(0.0f);
    glm::quat mRotation = glm::identity<glm::quat>();
    glm::vec3 mScaling = glm::vec3(1.0f);

    glm::mat4 mTranslationMatrix = glm::mat4(1.0f);
    glm::mat4 mRotationMatrix = glm::mat4(1.0f);
    glm::mat4 mScalingMatrix = glm::mat4(1.0f);

    glm::mat4 mParentNodeMatrix = glm::mat4(1.0f);
    glm::mat4 mLocalTRSMatrix = glm::mat4(1.0f);

    /* extra matrix to move model instances  around */
    glm::mat4 mRootTransformMatrix = glm::mat4(1.0f);
};

struct VertexIndexBuffer
{
    GLuint mVAO = 0;
    GLuint mVertexVBO = 0;
    GLuint mIndexVBO = 0;

    void init(OpenGL *opengl) 
    {
        opengl->glGenVertexArrays(1, &mVAO);
        opengl->glGenBuffers(1, &mVertexVBO);
        opengl->glGenBuffers(1, &mIndexVBO);

        opengl->glBindVertexArray(mVAO);

        opengl->glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO);

        opengl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, position));
        opengl->glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, color));
        opengl->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, normal));
        opengl->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, uv));
        opengl->glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT,   sizeof(OGLVertex), (void*) offsetof(OGLVertex, boneNumber));
        opengl->glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, boneWeight));

        opengl->glEnableVertexAttribArray(0);
        opengl->glEnableVertexAttribArray(1);
        opengl->glEnableVertexAttribArray(2);
        opengl->glEnableVertexAttribArray(3);
        opengl->glEnableVertexAttribArray(4);
        opengl->glEnableVertexAttribArray(5);

        opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexVBO);
        /* do NOT unbind index buffer here!*/

        opengl->glBindBuffer(GL_ARRAY_BUFFER, 0);
        opengl->glBindVertexArray(0);

        //Logger::log(1, "%s: VAO and VBOs initialized\n", __FUNCTION__);
    }

    void cleanup(OpenGL *opengl) 
    {
        opengl->glDeleteBuffers(1, &mIndexVBO);
        opengl->glDeleteBuffers(1, &mVertexVBO);
        opengl->glDeleteVertexArrays(1, &mVAO);
    }

    void uploadData(OpenGL *opengl, std::vector<OGLVertex> vertexData, std::vector<uint32_t> indices) 
    {
        if (vertexData.empty() || indices.empty()) {
            //Logger::log(1, "%s error: invalid data to upload (vertices: %i, indices: %i)\n", __FUNCTION__, vertexData.size(), indices.size());
            return;
        }

        opengl->glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO);
        opengl->glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(OGLVertex), &vertexData.at(0), GL_DYNAMIC_DRAW);
        opengl->glBindBuffer(GL_ARRAY_BUFFER, 0);

        opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexVBO);
        opengl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices.at(0), GL_DYNAMIC_DRAW);
        opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void bind(OpenGL *opengl) 
    {
        opengl->glBindVertexArray(mVAO);
    }

    void unbind(OpenGL *opengl) 
    {
        opengl->glBindVertexArray(0);
    }

    #if 0

    void draw(OpenGL *opengl, GLuint mode, unsigned int start, unsigned int num) 
    {
        opengl->glDrawArrays(mode, start, num);
    }

    void bindAndDraw(OpenGL *opengl, GLuint mode, unsigned int start, unsigned int num) 
    {
        bind();
        draw(mode, start, num);
        unbind();
    }

    void drawIndirect(OpenGL *opengl, GLuint mode, unsigned int num) 
    {
        opengl->glDrawElements(mode, num, GL_UNSIGNED_INT, 0);
    }

    void bindAndDrawIndirect(OpenGL *opengl, GLuint mode, unsigned int num) 
    {
        bind();
        drawIndirect(opengl, mode, num);
        unbind();
    }
    #endif

    void drawIndirectInstanced(OpenGL *opengl, GLuint mode, unsigned int num, int instanceCount) {
        opengl->glDrawElementsInstanced(mode, num, GL_UNSIGNED_INT, 0, instanceCount);
    }
    void bindAndDrawIndirectInstanced(OpenGL *opengl, GLuint mode, unsigned int num, int instanceCount) 
    {
        bind(opengl);
        drawIndirectInstanced(opengl, mode, num, instanceCount);
        unbind(opengl);
    }
     
};

/* transposes the matrix from Assimp to GL format */
glm::mat4 convertAiToGLM(aiMatrix4x4 inMat) {
  return {
    inMat.a1, inMat.b1, inMat.c1, inMat.d1,
    inMat.a2, inMat.b2, inMat.c2, inMat.d2,
    inMat.a3, inMat.b3, inMat.c3, inMat.d3,
    inMat.a4, inMat.b4, inMat.c4, inMat.d4
  };
}


struct Model
{
    u32 vertex_count;
    u32 triangle_count;
    Node *mRootNode;
    /* a map to find the node by name */
    std::unordered_map<std::string, Node*> mNodeMap{};
    /* and a 'flat' map to keep the order of insertation  */
    std::vector<Node*> mNodeList{};
    std::vector<OGLMesh> mModelMeshes;
    std::vector<VertexIndexBuffer> mVertexBuffers{};

    glm::mat4 mRootTransformMatrix = glm::mat4(1.0f);

    // map textures to external or internal texture names
    std::unordered_map<std::string, Texture *> mTextures{};
    Texture *mPlaceholderTexture = nullptr;
    Texture *mWhiteTexture = nullptr;


};

Node *createNode(std::string nodeName)
{
    Node* node = new Node();
    node->nodeName = nodeName;
    node->mParentNode = {};
    node->mChildNodes = {};

    node->mTranslation = glm::vec3(0.0f);
    node->mRotation = glm::identity<glm::quat>();
    node->mScaling = glm::vec3(1.0f);

    node->mTranslationMatrix = glm::mat4(1.0f);
    node->mRotationMatrix = glm::mat4(1.0f);
    node->mScalingMatrix = glm::mat4(1.0f);

    node->mParentNodeMatrix = glm::mat4(1.0f);
    node->mLocalTRSMatrix = glm::mat4(1.0f);

    /* extra matrix to move model instances  around */
    node->mRootTransformMatrix = glm::mat4(1.0f);

    return node;
}

bool processMesh(OpenGL *opengl, Mesh *myMesh, aiMesh* mesh, const aiScene* scene, std::string assetDirectory)
{
    myMesh->mMeshName = mesh->mName.C_Str();

  myMesh->mTriangleCount = mesh->mNumFaces;
  myMesh->mVertexCount = mesh->mNumVertices;

  //Logger::log(1, "%s: -- mesh '%s' has %i faces (%i vertices)\n", __FUNCTION__, myMesh->mMeshName.c_str(), mTriangleCount, mVertexCount);
  for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i) {
    if (mesh->HasVertexColors(i)) {
      //Logger::log(1, "%s: --- mesh has vertex colors in set %i\n", __FUNCTION__, i);
    }
  }
  if (mesh->HasNormals()) {
    //Logger::log(1, "%s: --- mesh has normals\n", __FUNCTION__);
  }
  for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
    if (mesh->HasTextureCoords(i)) {
      //Logger::log(1, "%s: --- mesh has texture cooords in set %i\n", __FUNCTION__, i);
    }
  }

  aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
  if (material) {
    aiString materialName = material->GetName();
    //Logger::log(1, "%s: - material found, name '%s'\n", __FUNCTION__, materialName.C_Str());

    if (mesh->mMaterialIndex >= 0) {
      // scan only for diifuse and scalar textures for a start
      std::vector<aiTextureType> supportedTexTypes = { aiTextureType_DIFFUSE, aiTextureType_SPECULAR };
      for (const auto& texType : supportedTexTypes) {
        unsigned int textureCount = material->GetTextureCount(texType);
        if (textureCount > 0) {
          //Logger::log(1, "%s: -- material '%s' has %i images of type %i\n", __FUNCTION__, materialName.C_Str(), textureCount, texType);
          for (unsigned int i = 0; i < textureCount; ++i) {
            aiString textureName;
            material->GetTexture(texType, i, &textureName);
            //Logger::log(1, "%s: --- image %i has name '%s'\n", __FUNCTION__, i, textureName.C_Str());

            std::string texName = textureName.C_Str();
            myMesh->mMesh.textures.insert({texType, texName});

            // do not try to load internal textures
            if (!texName.empty() && texName.find("*") != 0) {
              Texture *newTex = new Texture();
              std::string texNameWithPath = assetDirectory + '/' + texName;
              if (!newTex->loadTexture(opengl, texNameWithPath)) {
                // Logger::log(1, "%s error: could not load texture file '%s', skipping\n", __FUNCTION__, texNameWithPath.c_str());
                continue;
              }

              myMesh->mTextures.insert({texName, newTex});
            }
          }
        }
      }
    }

    aiColor4D baseColor(0.0f, 0.0f, 0.0f, 1.0f);
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == aiReturn_SUCCESS && myMesh->mTextures.empty()) {
        myMesh->mBaseColor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        myMesh->mMesh.usesPBRColors = true;
    }
  }

  for (unsigned int i = 0; i < myMesh->mVertexCount; ++i) {
    OGLVertex vertex;
    vertex.position.x = mesh->mVertices[i].x;
    vertex.position.y = mesh->mVertices[i].y;
    vertex.position.z = mesh->mVertices[i].z;


    if (mesh->HasVertexColors(0)) {
      vertex.color.r = mesh->mColors[0][i].r;
      vertex.color.g = mesh->mColors[0][i].g;
      vertex.color.b = mesh->mColors[0][i].b;
      vertex.color.a = mesh->mColors[0][i].a;
    } else {
      if (myMesh->mMesh.usesPBRColors) {
        vertex.color = myMesh->mBaseColor;
      } else {
        vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
      }
    }

    if (mesh->HasNormals()) {
      vertex.normal.x = mesh->mNormals[i].x;
      vertex.normal.y = mesh->mNormals[i].y;
      vertex.normal.z = mesh->mNormals[i].z;
    } else {
      vertex.normal = glm::vec3(0.0f);
    }

    if (mesh->HasTextureCoords(0)) {
      vertex.uv.x = mesh->mTextureCoords[0][i].x;
      vertex.uv.y = mesh->mTextureCoords[0][i].y;
    } else {
      vertex.uv = glm::vec2(0.0f);
    }

    myMesh->mMesh.vertices.emplace_back(vertex);
  }

  for (unsigned int i = 0; i < myMesh->mTriangleCount; ++i) {
    aiFace face = mesh->mFaces[i];
    myMesh->mMesh.indices.push_back(face.mIndices[0]);
    myMesh->mMesh.indices.push_back(face.mIndices[1]);
    myMesh->mMesh.indices.push_back(face.mIndices[2]);
  }


  #if 0
  if (mesh->HasBones()) {
    unsigned int numBones = mesh->mNumBones;
    //Logger::log(1, "%s: -- mesh has information about %i bones\n", __FUNCTION__, numBones);
    for (unsigned int boneId = 0; boneId < numBones; ++boneId) {
      std::string boneName = mesh->mBones[boneId]->mName.C_Str();
      unsigned int numWeights = mesh->mBones[boneId]->mNumWeights;
      //Logger::log(1, "%s: --- bone nr. %i has name %s, contains %i weights\n", __FUNCTION__, boneId, boneName.c_str(), numWeights);

      std::shared_ptr<AssimpBone> newBone = std::make_shared<AssimpBone>(boneId, boneName, convertAiToGLM(mesh->mBones[boneId]->mOffsetMatrix));
      mBoneList.push_back(newBone);

      for (unsigned int weight = 0; weight < numWeights; ++weight) {
        unsigned int vertexId = mesh->mBones[boneId]->mWeights[weight].mVertexId;
        float vertexWeight = mesh->mBones[boneId]->mWeights[weight].mWeight;

        glm::uvec4 currentIds = mMesh.vertices.at(vertexId).boneNumber;
        glm::vec4 currentWeights = mMesh.vertices.at(vertexId).boneWeight;

        /* insert weight and bone id into first free slot (weight => 0.0f) */
        for (unsigned int i = 0; i < 4; ++i) {
          if (currentWeights[i] == 0.0f) {
            currentIds[i] = boneId;
            currentWeights[i] = vertexWeight;

            /* skip to next weight */
            break;
          }
        }

        mMesh.vertices.at(vertexId).boneNumber = currentIds;
        mMesh.vertices.at(vertexId).boneWeight = currentWeights;

      }
    }
  }
  #endif

  return true;
}


void processNode(OpenGL *opengl, Model *model, Node* node, aiNode* aNode, const aiScene* scene, std::string assetDirectory) {
  std::string nodeName = aNode->mName.C_Str();
  //Logger::log(1, "%s: node name: '%s'\n", __FUNCTION__, nodeName.c_str());

  unsigned int numMeshes = aNode->mNumMeshes;
  if (numMeshes > 0) {
    //Logger::log(1, "%s: - node has %i meshes\n", __FUNCTION__, numMeshes);
    for (unsigned int i = 0; i < numMeshes; ++i) {
      aiMesh* modelMesh = scene->mMeshes[aNode->mMeshes[i]];

      Mesh mesh;
      processMesh(opengl, &mesh, modelMesh, scene, assetDirectory);

      model->mModelMeshes.emplace_back(mesh.mMesh);
      model->mTextures.merge(mesh.mTextures);

      /* avoid inserting duplicate bone Ids - meshes can reference the same bones */
      //std::vector<std::shared_ptr<AssimpBone>> flatBones = mesh.getBoneList();
      //for (const auto& bone : flatBones) {
      //  const auto iter = std::find_if(mBoneList.begin(), mBoneList.end(), [bone](std::shared_ptr<AssimpBone>& otherBone) { return bone->getBoneId() == otherBone->getBoneId(); });
      //  if (iter == mBoneList.end()) {
      //    mBoneList.emplace_back(bone);
      //  }
      //}
    }
  }

  model->mNodeMap.insert({nodeName, node});
  model->mNodeList.emplace_back(node);

  unsigned int numChildren = aNode->mNumChildren;
  //Logger::log(1, "%s: - node has %i children \n", __FUNCTION__, numChildren);

  for (unsigned int i = 0; i < numChildren; ++i) {
    std::string childName = aNode->mChildren[i]->mName.C_Str();

    //Node childNode = node->addChild(childName);
    Node *childNode = createNode(childName);
    childNode->mParentNode = node;
    node->mChildNodes.push_back(childNode);
    /* should be: node->last = createNode(childName)
    node->last->next = createNode(childName);
    node->last = node->last->next;
    */

    processNode(opengl, model, childNode, aNode->mChildren[i], scene, assetDirectory);
  }
}

internal void
drawInstanced(Model *model, OpenGL *opengl, u32 instanceCount)
{
    // TODO por que cuando tenia comentadas las texturas defaulteaba a las otras texturas? en algun lado se cargaron, donde?
    // This was happening because I forgot to unbind the texture I use for UI and also I got confused because the woman and the ui
    // was using similar colors, or the same, but not quite so I thought it was using the woman's texture but it was not, it was using the cube.
    // So the fix was to unbind the texture in end_ui_frame
    for (u32 i = 0; i < model->mModelMeshes.size(); ++i) 
    {
        OGLMesh& mesh = model->mModelMeshes.at(i);
        // find diffuse texture by name

        Texture *diffuseTex = nullptr;
        auto diffuseTexName = mesh.textures.find(aiTextureType_DIFFUSE);
        if (diffuseTexName != mesh.textures.end()) {
        auto diffuseTexture = model->mTextures.find(diffuseTexName->second);
        if (diffuseTexture != model->mTextures.end()) {
            diffuseTex = diffuseTexture->second;
        }
        }

        opengl->glActiveTexture(GL_TEXTURE0);

        if (diffuseTex) {
          diffuseTex->bind();
        } else {
          if (mesh.usesPBRColors) {
            model->mWhiteTexture->bind();
          } else {
            model->mPlaceholderTexture->bind();
          }
        }

        //model->mVertexBuffers.at(i).bindAndDrawIndirectInstanced(opengl, GL_TRIANGLES, mesh.indices.size(), instanceCount);

        VertexIndexBuffer model_buffer = model->mVertexBuffers.at(i);
        model_buffer.bind(opengl);
        #if 0
            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
        #else
            opengl->glDrawElementsInstanced(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0, 1);
        #endif
        model_buffer.unbind(opengl);

        if (diffuseTex) {
          diffuseTex->unbind();
        } else {
          if (mesh.usesPBRColors) {
            model->mWhiteTexture->unbind();
          } else {
            model->mPlaceholderTexture->unbind();
          }
        }
    }
}


struct PlaceholderState
{
    // TODO probably should have its own memory. After I remove std first!
    glm::mat4 persp_proj;
    Model *model;
    u32 nonskinned_pid;
    u32 skinned_pid;
};

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



// NOTE before pulling them out I have to sort out the globals being used here!
void opengl_render (OpenGL *opengl, Camera *curr_camera, u32 cubeVAO,
    Shader skinning_shader, MeshBox floor_meshbox, std::vector<MeshBox> boxes, UIRenderGroup* render_group, PlaceholderState *placeholder_state)
{
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glm::mat4 view = curr_camera->GetViewMatrix();

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
            Model *woman = placeholder_state->model;

            opengl->glUseProgram(placeholder_state->nonskinned_pid);
            // generic data for all models!
            std::vector<glm::mat4> matrixData;
            matrixData.emplace_back(view);
            matrixData.emplace_back(placeholder_state->persp_proj);

            mUniformBuffer.uploadUboData(opengl, matrixData, 0);

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
            opengl->glUseProgram(0);
        }

        {
            // render ui
            // TODO this should only be done at resizes
            glm::mat4 orthog = glm::ortho(0.0f, f32(SRC_WIDTH), f32(SRC_HEIGHT), 0.0f, -1.0f, 1.0f);
            begin_ui_frame(opengl, render_group);
            opengl->glUniformMatrix4fv(opengl->ui_render_group->ortho_proj, 1, GL_FALSE, &orthog[0][0]);
            // TODO see wtf is a 0 there? Why is the texture sampler for?
            opengl->glUniform1i(opengl->ui_render_group->texture_sampler, 0);
            // TODO investigate if i have to set this value for the uniforms again or not!. 

            glDrawElements(GL_TRIANGLES, render_group->index_count, GL_UNSIGNED_SHORT, 0); //era 18
            end_ui_frame(opengl);
        }

        opengl->glUseProgram(0);
}


int main() {
    aim_profiler_begin();
    global_w32_window = os_win32_open_window("opengl", SRC_WIDTH, SRC_HEIGHT, win32_main_callback, 1);
	
	arena_init(&g_arena, mb(2));
	arena_init(&g_transient_arena, mb(2));

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
    UIRenderGroup *ui_render_group = opengl->ui_render_group;
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
        const glm::vec3 T0v = {100.0f,  500.0f, 0.0f};
        const glm::vec3 T1v = {300.0f,  500.0f, 0.0f};
        const glm::vec3 T2v = {300.0f,  300.0f, 0.0f};
        const glm::vec3 T3v = {100.0f,  300.0f, 0.0f};

        const glm::vec3 rect_points[4] = {T0v, T1v, T2v, T3v};
        push_rect(ui_render_group, rect_points);
        glm::vec3 tri_points[3] = {glm::vec3(500.0f, 500.0f, 0.0f), glm::vec3(600.0f, 500.0f, 0.0f), glm::vec3(450.0f, 300.0f, 0.0f)};
        u16 tri_indices[3] = {0, 1, 2};
        push_triangle(ui_render_group, tri_points, tri_indices);
    }

    // uniform init
    size_t uniform_matrix_buffer_size = 3 * sizeof(glm::mat4);
    mUniformBuffer.init(opengl, uniform_matrix_buffer_size);

    // ssbo init!
    mWorldPosBuffer.mBufferSize = 256;
    opengl->glGenBuffers(1, &mWorldPosBuffer.mShaderStorageBuffer);
    opengl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, mWorldPosBuffer.mShaderStorageBuffer);
    opengl->glBufferData(GL_SHADER_STORAGE_BUFFER, mWorldPosBuffer.mBufferSize, nullptr, GL_DYNAMIC_COPY);
    opengl->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // TODO see todos
    Shader skinning_shader{};
    shader_init(&skinning_shader, opengl, str8("skel_shader-2.vs.glsl"), str8("6.multiple_lights.fs.glsl"));
    
    PlaceholderState *placeholder_state = arena_push_size(&g_arena, PlaceholderState, 1);
    placeholder_state->nonskinned_pid = create_program(opengl, str8("assimp.vert"), str8("assimp.frag"));
    placeholder_state->skinned_pid = create_program(opengl, str8("assimp_skinning.vert"), str8("assimp_skinning.frag"));
    placeholder_state->persp_proj = glm::perspective(glm::radians(curr_camera->zoom), (float)SRC_WIDTH / (float)SRC_HEIGHT, 0.1f, 10000.0f);
    placeholder_state->model = new Model();
    {
        Model *model = placeholder_state->model;
        // w1 models
        Assimp::Importer importer;
        std::string model_filename = "E:/Mastering-Cpp-Game-Animation-Programming/chapter01/01_opengl_assimp/assets/woman/Woman.gltf";

        const aiScene *scene = importer.ReadFile(model_filename, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_ValidateDataStructure);
        AssertGui(scene && scene->mRootNode, "Error loading model, probably not found!");

        model->vertex_count = 0;
        model->triangle_count = 0;
        model->mRootNode = 0;
        
        u32 meshes_count = scene->mNumMeshes;
        for (u32 mesh_idx = 0; mesh_idx < meshes_count; mesh_idx++)
        {
            u32 vertices_count = scene->mMeshes[mesh_idx]->mNumVertices;
            u32 faces_count = scene->mMeshes[mesh_idx]->mNumFaces;

            model->vertex_count += vertices_count;
            model->triangle_count += faces_count;
        }

        if(scene->HasTextures())
        {
            u32 num_textures = scene->mNumTextures;
            for(u32 t = 0; t < num_textures; t++)
            {
                std::string texName = scene->mTextures[t]->mFilename.C_Str();
                i32 height = scene->mTextures[t]->mHeight;
                i32 width = scene->mTextures[t]->mWidth;
                aiTexel *data = scene->mTextures[t]->pcData;

                Texture *newTex = new Texture();
                if(!newTex->loadTexture(opengl, texName, data, width, height))
                {
                    return false;
                }

                std::string internalTexName = "*" + std::to_string(t);
                model->mTextures.insert({internalTexName, newTex});
            }
        }
        // ... yunk
        // ... yunk
        /* add a white texture in case there is no diffuse tex but colors */
        model->mWhiteTexture = new Texture();
        std::string whiteTexName = "src/samples/render_example/textures/white.png";
        if (!model->mWhiteTexture->loadTexture(opengl, whiteTexName)) {
            //Logger::log(1, "%s error: could not load white default texture '%s'\n", __FUNCTION__, whiteTexName.c_str());
            // Por que tendria que retornar? zii
            //return false;
        }

        /* add a placeholder texture in case there is no diffuse tex */
        model->mPlaceholderTexture = new Texture();
        std::string placeholderTexName = "src/samples/render_example/textures/missing_tex.png";
        if (!model->mPlaceholderTexture->loadTexture(opengl, placeholderTexName)) {
            //Logger::log(1, "%s error: could not load placeholder texture '%s'\n", __FUNCTION__, placeholderTexName.c_str());
            // Por que tendria que retornar? zii
            //return false;
        }

        aiNode* rootNode = scene->mRootNode;
        std::string rootNodeName = rootNode->mName.C_Str();
        model->mRootNode = createNode(rootNodeName);
        //Logger::log(1, "%s: root node name: '%s'\n", __FUNCTION__, rootNodeName.c_str());

        // TODO is this the texture directory?
        std::string assetDirectory = "E:/Mastering-Cpp-Game-Animation-Programming/chapter01/01_opengl_assimp/assets/woman";
        processNode(opengl, model, model->mRootNode, rootNode, scene, assetDirectory);

        //Logger::log(1, "%s: ... processing nodes finished...\n", __FUNCTION__);

        for (const auto& node : model->mNodeList) {
            std::string nodeName = node->nodeName;
            int x = 1;
            //const auto boneIter = std::find_if(mBoneList.begin(), mBoneList.end(), [nodeName](std::shared_ptr<AssimpBone>& bone) { return bone->getBoneName() == nodeName; });
            //if (boneIter != mBoneList.end()) {
            //mBoneOffsetMatrices.insert({nodeName, mBoneList.at(std::distance(mBoneList.begin(), boneIter))->getOffsetMatrix()});
            //}
        }

        /* create vertex buffers for the meshes */
        for (const auto& mesh : model->mModelMeshes) {
            VertexIndexBuffer buffer;
            buffer.init(opengl);
            buffer.uploadData(opengl, mesh.vertices, mesh.indices);
            model->mVertexBuffers.emplace_back(buffer);
        }

        #if 0
        /* animations */
        unsigned int numAnims = scene->mNumAnimations;
        for (unsigned int i = 0; i < numAnims; ++i) {
            aiAnimation* animation = scene->mAnimations[i];

            Logger::log(1, "%s: -- animation clip %i has %i skeletal channels, %i mesh channels, and %i morph mesh channels\n",
            __FUNCTION__, i, animation->mNumChannels, animation->mNumMeshChannels, animation->mNumMorphMeshChannels);

            std::shared_ptr<AssimpAnimClip> animClip = std::make_shared<AssimpAnimClip>();
            animClip->addChannels(animation);
            if (animClip->getClipName().empty()) {
            animClip->setClipName(std::to_string(i));
            }
            mAnimClips.emplace_back(animClip);
        }

        mModelFilenamePath = modelFilename;
        mModelFilename = std::filesystem::path(modelFilename).filename().generic_string();
        #endif

        /* get root transformation matrix from model's root node */
        model->mRootTransformMatrix = convertAiToGLM(rootNode->mTransformation);

        //Logger::log(1, "%s: - model has a total of %i texture%s\n", __FUNCTION__, mTextures.size(), mTextures.size() == 1 ? "" : "s");
        //Logger::log(1, "%s: - model has a total of %i bone%s\n", __FUNCTION__, mBoneList.size(), mBoneList.size() == 1 ? "" : "s");
        //Logger::log(1, "%s: - model has a total of %i animation%s\n", __FUNCTION__, numAnims, numAnims == 1 ? "" : "s");

        //Logger::log(1, "%s: successfully loaded model '%s' (%s)\n", __FUNCTION__, modelFilename.c_str(), mModelFilename.c_str());
    }
    // add model to model list here?


    // shader

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


//  unsigned int quad_vbo, quad_vao;
//	opengl->glGenVertexArrays(1, &quad_vao);
//	opengl->glGenBuffers(1, &quad_vbo);


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

    while (global_w32_window.is_running)
	{
        LONGLONG now = aim_timer_get_os_time();
        LONGLONG dt_long = now - last_frame;
        last_frame = now;
        f32 dt = aim_timer_ticks_to_sec(dt_long, frequency);
        global_input.dt = dt;

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
        opengl_render(opengl, curr_camera, cubeVAO, skinning_shader, floor_meshbox, boxes, ui_render_group, placeholder_state);
        //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);

        input_update(&global_input);
        // TODO see when hdc needs to be get again?
        // Probably it doesnt, and not releasing casued a huge memory leak wich was not showing in task manager!
        HDC hdc = GetDC(global_w32_window.handle);
        SwapBuffers(hdc);
        ReleaseDC(global_w32_window.handle, hdc);
    }
	aim_profiler_end();
	aim_profiler_print();
}