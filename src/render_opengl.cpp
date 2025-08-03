#include <windows.h>
#include <gl/GL.h>


struct render_state
{
  HDC context;
};



typedef char        GLchar;
typedef ptrdiff_t   GLsizeiptr;
typedef ptrdiff_t   GLintptr;


#pragma region OpenGL Extensions
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#define GL_DEBUG_OUTPUT                   0x92E0
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_LINK_STATUS                    0x8B82
#define GL_ARRAY_BUFFER                   0x8892
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE8                       0x84C8
#define GL_TEXTURE9                       0x84C9
#define GL_TEXTURE10                      0x84CA
#define GL_TEXTURE11                      0x84CB
#define GL_TEXTURE12                      0x84CC
#define GL_TEXTURE13                      0x84CD
#define GL_TEXTURE14                      0x84CE
#define GL_TEXTURE15                      0x84CF
#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_SWAP_METHOD_ARB               0x2007
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_ACCELERATION_ARB              0x2003
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_ALPHA_BITS_ARB                0x201B
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_FULL_ACCELERATION_ARB         0x2027
#define WGL_SWAP_COPY_ARB                 0x2029
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_FLAGS_ARB             0x2094
#define WGL_CONTEXT_DEBUG_BIT_ARB         0x00000001
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#pragma endregion


#pragma region OpenGL Functions
typedef void (APIENTRY  *GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);

#define GL_FUNC_SIGNATURE(ret, name, ...) typedef ret WINAPI glfunc_##name(__VA_ARGS__);
#define REQUIRED_OPENGL_FUNCTIONS                                                                                                                           \
    GL_FUNC_SIGNATURE(GLuint, glCreateProgram, void)                                                                                                        \
    GL_FUNC_SIGNATURE(GLuint, glCreateShader, GLenum shaderType)                                                                                            \
    GL_FUNC_SIGNATURE(bool, wglSwapIntervalEXT, int interval)                                                                                               \
    GL_FUNC_SIGNATURE(int,  wglGetSwapIntervalEXT, void)                                                                                                \
    GL_FUNC_SIGNATURE(void, glUniformMatrix4fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)                                   \
    GL_FUNC_SIGNATURE(void, glUniform3fv, GLint location, GLsizei count, const GLfloat *value)                                                              \
    GL_FUNC_SIGNATURE(void, glCompileShader, GLuint shader)                                                                                                 \
    GL_FUNC_SIGNATURE(void, glShaderSource, GLuint shader, GLsizei count, const GLchar** string, const GLint *length)                                       \
    GL_FUNC_SIGNATURE(void, glGetShaderiv, GLuint shader, GLenum pname, GLint* params)                                                                      \
    GL_FUNC_SIGNATURE(void, glGetShaderInfoLog, GLuint shader, GLsizei maxLength, GLsizei* length, char *infoLog)                                           \
    GL_FUNC_SIGNATURE(void, glAttachShader, GLuint program, GLuint shader)                                                                                  \
    GL_FUNC_SIGNATURE(void, glDetachShader, GLuint program, GLuint shader)                                                                                  \
    GL_FUNC_SIGNATURE(void, glLinkProgram, GLuint program)                                                                                                  \
    GL_FUNC_SIGNATURE(void, glGetProgramiv, GLuint program, GLenum pname, GLint *params)                                                                    \
    GL_FUNC_SIGNATURE(void, glDeleteShader, GLuint shader)                                                                                                  \
    GL_FUNC_SIGNATURE(void, glGenVertexArrays, GLsizei n, GLuint *arrays)                                                                                   \
    GL_FUNC_SIGNATURE(void, glGenBuffers, GLsizei n, GLuint* buffers)                                                                                       \
    GL_FUNC_SIGNATURE(void, glBindVertexArray, GLuint array)                                                                                                \
    GL_FUNC_SIGNATURE(void, glBindBuffer, GLenum target, GLuint buffer)                                                                                     \
    GL_FUNC_SIGNATURE(void, glBufferData, GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)                                                 \
    GL_FUNC_SIGNATURE(void, glVertexAttribPointer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer)      \
    GL_FUNC_SIGNATURE(void, glEnableVertexAttribArray, GLuint index)                                                                                        \
    GL_FUNC_SIGNATURE(void, glUseProgram, GLuint program)                                                                                                   \
    GL_FUNC_SIGNATURE(void, glDeleteVertexArrays, GLsizei n, const GLuint *arrays)                                                                          \
    GL_FUNC_SIGNATURE(void, glGetProgramInfoLog, GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog)                                       \
    GL_FUNC_SIGNATURE(void, glDeleteBuffers, GLsizei n, const GLuint * buffers)                                                                             \
    GL_FUNC_SIGNATURE(void, glDeleteProgram, GLuint program)                                                                                                \
    GL_FUNC_SIGNATURE(GLint, glGetUniformLocation, GLuint program, const GLchar *name)                                                                      \
    GL_FUNC_SIGNATURE(void, glGenerateMipmap, GLenum target)                                                                                                \
    GL_FUNC_SIGNATURE(void, glActiveTexture, GLenum texture)                                                                                                \
    GL_FUNC_SIGNATURE(void, glUniform1i, GLint location, GLint v0)                                                                                          \
    GL_FUNC_SIGNATURE(void, glUniform4f, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)                                                    \
    GL_FUNC_SIGNATURE(void, glBufferSubData, GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data)                                           \
    GL_FUNC_SIGNATURE(void, glDebugMessageCallback, GLDEBUGPROC callback, void *userParam) \


    // end of list 
REQUIRED_OPENGL_FUNCTIONS
#undef GL_FUNC_SIGNATURE
#define GL_FUNC_SIGNATURE(ret, name, ...) glfunc_##name * name;
REQUIRED_OPENGL_FUNCTIONS
#undef GL_FUNC_SIGNATURE
bool opengl_load_functions(void)
{
    HMODULE module = LoadLibraryExW(L"opengl32.dll", NULL, 0 );
    if (!module) {
        OutputDebugStringW(L"opengl32.dll not found.\n");
        return false;
    }
    #define GL_FUNC_SIGNATURE(ret, name, ...)                                                       \
            name = (glfunc_##name *) wglGetProcAddress(#name);                                      \
            if (                                                                                    \
                name == 0 ||                                                                        \
                (name == (glfunc_##name *) 0x1) ||                                                             \
                (name == (glfunc_##name *) 0x2) ||                                                             \
                (name == (glfunc_##name *) 0x3) ||                                                             \
                (name == (glfunc_##name *) -1)                                                                 \
            )                                                                                       \
            {                                                                                       \
                OutputDebugStringW(L"wglGetProcAddress failed to find " #name "\n");                 \
                name = (glfunc_##name *) GetProcAddress(module, #name);                             \
            }                                                                                       \
            if (!name)                                                                              \
            {                                                                                       \
                OutputDebugStringW(L"Function " #name " couldn't be loaded from opengl32.dll\n");  \
                return false;                                                                       \
            }
        REQUIRED_OPENGL_FUNCTIONS
    #undef GL_FUNC_SIGNATURE
    return true;
}
#pragma endregion



render_state render_init(HWND handle)
{
  render_state state = {};

  state.context = GetDC(handle);
  // Set pixel format
  PIXELFORMATDESCRIPTOR requestedPixelFormat = {}; 
  requestedPixelFormat.nSize = sizeof(requestedPixelFormat); 
  requestedPixelFormat.nVersion = 1; 
  requestedPixelFormat.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER; 
  requestedPixelFormat.cColorBits = 32; 
  requestedPixelFormat.cAlphaBits = 8;
  int givenPixelFormat = ChoosePixelFormat(state.context, &requestedPixelFormat);
  // We got a format from windows
  PIXELFORMATDESCRIPTOR givenFormatFilled = {};
  DescribePixelFormat(state.context, givenPixelFormat, sizeof(givenPixelFormat), &givenFormatFilled);
  bool successfullySetPixelFormat = SetPixelFormat(state.context, givenPixelFormat, &givenFormatFilled);
  // Create context
  HGLRC OpenGLRC = wglCreateContext(state.context);
  bool successfullySetContext = wglMakeCurrent(state.context, OpenGLRC);
  // Load OpenGL functions
  opengl_load_functions();
  // Set the viewport
  RECT rect;
  GetClientRect(handle, &rect);
  glViewport(0, 0, rect.right, rect.bottom);
  return state;
}

void frame_render(render_state *state)
{
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  SwapBuffers(state->context);
}

void render_close(render_state *state)
{
  HWND handle = WindowFromDC(state->context);
  ReleaseDC(handle, state->context);
}
