#include <gl/gl.h>
#include<windows.h>

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

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
    glDrawArrays(GL_TRIANGLES, 0, 36);


#define OpenGLGetFunction(name) (OpenGLType_##name) wglGetProcAddress(#name)