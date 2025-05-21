#include<windows.h>
#include <gl/gl.h>

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091 // casey
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092 // casey
#define WGL_CONTEXT_FLAGS_ARB                   0x2094 // casey
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126 // casey
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001 // casey

// these are not used in this example!
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x00000002 
#define WGL_CONTEXT_DEBUG_BIT_ARB               0x00000001 // casey internal
#define ERROR_INVALID_PROFILE_ARB               0x2096
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPUTE_SHADER                 0x91B9
#define GL_PROGRAM                        0x82E2
#define GL_ARRAY_BUFFER                   0x8892
#define GL_UNIFORM_BUFFER                 0x8A11
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_COPY                   0x88EA
#define GL_COMPILE_STATUS                 0x8B81
#define GL_SHADER_STORAGE_BUFFER          0x90D2
#define GL_LINK_STATUS                    0x8B82
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_UNSIGNED_INT                   0x1405
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TEXTURE0                       0x84C0
#define GL_SRGB8_ALPHA8                   0x8C43


#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148
#define GL_DEBUG_TYPE_PORTABILITY         0x824F
#define GL_DEBUG_TYPE_PERFORMANCE         0x8250
#define GL_DEBUG_TYPE_MARKER              0x8268
#define GL_DEBUG_TYPE_OTHER               0x8251
#define GL_DEBUG_TYPE_PUSH_GROUP          0x8269
#define GL_DEBUG_TYPE_POP_GROUP           0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B
#define GL_DEBUG_OUTPUT                   0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242

#define GL_DEBUG_SOURCE_API               0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM     0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER   0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY       0x8249
#define GL_DEBUG_SOURCE_APPLICATION       0x824A
#define GL_DEBUG_SOURCE_OTHER             0x824B
#define GL_DEBUG_TYPE_ERROR               0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR  0x824E

#define GL_R8                             0x8229
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;
typedef void (APIENTRY  *GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);

#define OpenGLDefineFunction(name, ret, ...) \
 typedef ret (WINAPI *OpenGLType_##name) (__VA_ARGS__); \
 OpenGLType_##name name = 0;

OpenGLDefineFunction(wglCreateContextAttribsARB, HGLRC, HDC hDC, HGLRC hShareContext, const int *attribList);
OpenGLDefineFunction(wglGetExtensionsStringEXT, const char *, void);
OpenGLDefineFunction(wglGetSwapIntervalEXT, int, void);
OpenGLDefineFunction(wglSwapIntervalEXT, BOOL, int interval);
OpenGLDefineFunction(glGenVertexArrays, void, GLsizei n, GLuint *arrays);
OpenGLDefineFunction(glBindVertexArray, void, GLuint array);
OpenGLDefineFunction(glDeleteVertexArrays, void, GLsizei n, const GLuint *arrays);

OpenGLDefineFunction(glGenBuffers, void, GLsizei n, GLuint *buffers);
OpenGLDefineFunction(glBindBuffer, void, GLenum target, GLuint buffer);
OpenGLDefineFunction(glBufferData, void, GLenum target, GLsizeiptr size, const void *data, GLenum usage);
OpenGLDefineFunction(glDeleteBuffers, void, GLsizei n, const GLuint *buffers);
OpenGLDefineFunction(glVertexAttribPointer, void, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
OpenGLDefineFunction(glVertexAttribIPointer, void, GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
OpenGLDefineFunction(glEnableVertexAttribArray, void, GLuint index);

OpenGLDefineFunction(glCreateShader, GLuint, GLenum type);
OpenGLDefineFunction(glCompileShader, void, GLuint shader);
OpenGLDefineFunction(glShaderSource, void, GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
OpenGLDefineFunction(glCreateProgram, GLuint, void);
OpenGLDefineFunction(glAttachShader, void, GLuint program, GLuint shader);
OpenGLDefineFunction(glLinkProgram, void, GLuint program);
OpenGLDefineFunction(glDeleteShader, void, GLuint shader);
OpenGLDefineFunction(glDeleteProgram, void, GLuint program);
OpenGLDefineFunction(glUseProgram, void, GLuint program);
OpenGLDefineFunction(glGetShaderiv, void, GLuint shader, GLenum pname, GLint *params);
OpenGLDefineFunction(glGetShaderInfoLog, void, GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
OpenGLDefineFunction(glGetProgramiv, void, GLuint program, GLenum pname, GLint *params);
OpenGLDefineFunction(glGetProgramInfoLog, void, GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);

OpenGLDefineFunction(glUniform1i, void, GLint location, GLint v0);
OpenGLDefineFunction(glUniform1f, void, GLint location, GLfloat v0);
OpenGLDefineFunction(glUniform2fv, void, GLint location, GLsizei count, const GLfloat *value);
OpenGLDefineFunction(glUniform2f, void, GLint location, GLfloat v0, GLfloat v1);
OpenGLDefineFunction(glUniform3fv, void, GLint location, GLsizei count, const GLfloat *value);
OpenGLDefineFunction(glUniform3f, void, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
OpenGLDefineFunction(glUniform4fv, void, GLint location, GLsizei count, const GLfloat *value);
OpenGLDefineFunction(glUniform4f, void, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
OpenGLDefineFunction(glUniformMatrix2fv, void, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
OpenGLDefineFunction(glUniformMatrix3fv, void, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
OpenGLDefineFunction(glUniformMatrix4fv, void, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
OpenGLDefineFunction(glGetUniformLocation, GLint, GLuint program, const GLchar *name);

OpenGLDefineFunction(glActiveTexture, void, GLenum texture);
OpenGLDefineFunction(glGenerateMipmap, void, GLenum target);

OpenGLDefineFunction(glDrawElementsInstanced, void, GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
OpenGLDefineFunction(glBufferSubData, void, GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
OpenGLDefineFunction(glBindBufferRange, void, GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);

OpenGLDefineFunction(glDebugMessageCallback, void, GLDEBUGPROC callback, const void *userParam);
OpenGLDefineFunction(glDebugMessageControl, void, GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);


#define OpenGLGetFunction(name) (OpenGLType_##name) wglGetProcAddress(#name)