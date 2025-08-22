#include <windows.h>
#include <gl/GL.h>


struct render_state
{
  HDC context;
};

struct render_program
{
  GLuint vao;
  GLuint shader_program;
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
#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148
#define GL_DEBUG_SOURCE_APPLICATION       0x824A
#define GL_DEBUG_TYPE_ERROR               0x824C
#define GL_PROGRAM_POINT_SIZE             0x8642
#pragma endregion


#pragma region OpenGL Functions
// Unique windows functions not supposed to be loaded with the others.
typedef BOOL WINAPI glfunc_wglChoosePixelFormatARB(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC WINAPI glfunc_wglCreateContextAttribsARB (HDC hDC, HGLRC hShareContext, const int *attribList);


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
    GL_FUNC_SIGNATURE(void, glDebugMessageCallback, GLDEBUGPROC callback, void *userParam)                                                                  \
    GL_FUNC_SIGNATURE(void, glDebugMessageControl,	GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled)       \
    GL_FUNC_SIGNATURE(void, glDebugMessageInsert, GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char *message)               \



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


#ifdef _DEBUG

static void APIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user)
{
  if(severity == GL_DEBUG_SEVERITY_LOW || 
     severity == GL_DEBUG_SEVERITY_MEDIUM ||
     severity == GL_DEBUG_SEVERITY_HIGH)
  {
    char sentence_buffer[8192] = {0};
    i32 char_written_count = sprintf_s(sentence_buffer, sizeof(sentence_buffer), "OpenGL Error: %s \033[0m", message);
    ASSERT(false, sentence_buffer);
  }
  /*
  else
  {
    printf("OpenGL Error: %s\n", message);
  }
  */
}

#endif

render_state render_init(platform_window *window)
{
  render_state state = {};

  glfunc_wglChoosePixelFormatARB *wglChoosePixelFormatARB = 0;
  glfunc_wglCreateContextAttribsARB *wglCreateContextAttribsARB = 0;

  HWND handle = window->handle;
  // Use first window to load OpenGL stuff
  {
    HDC fakeDC = GetDC(handle);
    PIXELFORMATDESCRIPTOR pixelFormat = {0};
    pixelFormat.nSize = sizeof(pixelFormat);
    pixelFormat.nVersion = 1;
    pixelFormat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixelFormat.iPixelType = PFD_TYPE_RGBA;
    pixelFormat.cColorBits = 32;
    pixelFormat.cAlphaBits = 8;
    pixelFormat.cDepthBits = 24;
  
    int chosenPixelFormat = ChoosePixelFormat(fakeDC, &pixelFormat);
    ASSERT(chosenPixelFormat != 0, "ERROR: Failed to choose pixel format descriptor.");

    int isPixelFormatSet = SetPixelFormat(fakeDC, chosenPixelFormat, &pixelFormat);
    ASSERT(isPixelFormatSet != 0, "ERROR: Failed to set the pixel format descriptor.");

    HGLRC fakeRC = wglCreateContext(fakeDC);
    ASSERT(fakeRC != 0, "ERROR: Failed to create fake rendering context.");

    int isFakeCurrent = wglMakeCurrent(fakeDC, fakeRC);
    ASSERT(isFakeCurrent != 0, "ERROR: Failed to make the OpenGL rendering context the current rendering context.");

    wglChoosePixelFormatARB    = (glfunc_wglChoosePixelFormatARB*) wglGetProcAddress("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB = (glfunc_wglCreateContextAttribsARB*) wglGetProcAddress("wglCreateContextAttribsARB");
    bool loading_failed = (wglChoosePixelFormatARB == 0 || wglCreateContextAttribsARB == 0);
    ASSERT(loading_failed == false, "ERROR: Failed to load OpenGL functions.");
    wglMakeCurrent(fakeDC, 0);
    wglDeleteContext(fakeRC);
    ReleaseDC(handle, fakeDC);
    DestroyWindow(handle);
    UnregisterClassA("WINDOW_CLASS", window->instance);
    window->handle = nullptr;
    window->instance = nullptr;
  }

  // Create the real window
  {
    // Re-create the window
    window_init(window);
    handle = window->handle;
    // Get the device context
    state.context = GetDC(handle);
    ASSERT(state.context != 0, "ERROR: Failed to get device context.");
    const int pixelAttribs[] =
    {
      WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
      WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
      WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
      WGL_SWAP_METHOD_ARB,    WGL_SWAP_COPY_ARB,
      WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
      WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
      WGL_COLOR_BITS_ARB,     32,
      WGL_ALPHA_BITS_ARB,     8,
      WGL_DEPTH_BITS_ARB,     24,
      0 // Terminate with 0, otherwise OpenGL will throw an Error!
    };
    UINT numPixelFormats;
    int pixelFormat = 0;
    BOOL chosenPixelFormatARB = wglChoosePixelFormatARB(
      state.context,
      pixelAttribs,
      0, // Float List
      1, // Max Formats
      &pixelFormat,
      &numPixelFormats
    );
    ASSERT(chosenPixelFormatARB != 0, "ERROR: Failed to wglChoosePixelFormatARB");
    PIXELFORMATDESCRIPTOR pixelFormatDescriptor = {0};
    DescribePixelFormat(state.context, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pixelFormatDescriptor);
    BOOL isPixelFormatSet = SetPixelFormat(state.context, pixelFormat, &pixelFormatDescriptor);
    ASSERT(isPixelFormatSet != 0, "ERROR: Failed to set the pixel format.");
    const int contextAttribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB, 
        WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB, 
        WGL_CONTEXT_DEBUG_BIT_ARB,
        0 // Terminate the Array
    };
    HGLRC renderingContext = wglCreateContextAttribsARB(state.context, 0, contextAttribs);
    ASSERT(renderingContext != 0, "ERROR: Failed to create rendering context.");
    BOOL isContextSet = wglMakeCurrent(state.context, renderingContext);
    ASSERT(isContextSet != 0, "ERROR: Failed to set the device and rendering context.");
  }
  // Load OpenGL functions
  opengl_load_functions();
  // Enable debug function
  glDebugMessageCallback(&gl_debug_callback, nullptr);
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  /*
  // Testing OpenGL debugging
  glDebugMessageInsert(
    GL_DEBUG_SOURCE_APPLICATION,
    GL_DEBUG_TYPE_ERROR,
    1,
    GL_DEBUG_SEVERITY_HIGH,
    -1,
    "Manual test message\n"
  );
  */

  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
  // Set the viewport
  RECT rect;
  GetClientRect(handle, &rect);
  glViewport(0, 0, rect.right, rect.bottom);
  return state;
}


void point_setup(arena *scratch, render_program *prog)
{
  GLuint point_vbo = 0;
  
  size_t byte_count;
  // Vertex shader source
  ASSERT(file_exists("shaders\\points.vert") == true, "ERROR Shader not found.");
  const char* vertex_shader_source = read_file("shaders\\points.vert", scratch, &byte_count);

  // Fragment shader source  
  const char* fragment_shader_source = read_file("shaders\\points.frag", scratch, &byte_count);

  // Create vertex shader
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);

  // Create fragment shader
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
  glCompileShader(fragment_shader);

  // Create shader program
  prog->shader_program = glCreateProgram();
  glAttachShader(prog->shader_program, vertex_shader);
  glAttachShader(prog->shader_program, fragment_shader);
  glLinkProgram(prog->shader_program);

  // Clean up shaders
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  // Point at center (0,0,0) in normalized device coordinates
  float point_vertex[] = {
    0.0f, 0.0f, 0.0f
  };

  // Generate VAO and VBO
  glGenVertexArrays(1, &prog->vao);
  glGenBuffers(1, &point_vbo);

  glBindVertexArray(prog->vao);
  glBindBuffer(GL_ARRAY_BUFFER, point_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(point_vertex), point_vertex, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Enable point size control from vertex shader
  glEnable(GL_PROGRAM_POINT_SIZE);
}


void tri_setup(arena *scratch, render_program *prog)
{
  // Triangle
  /*
  f32 verts[] = {
    -0.5f, -0.5f, 0.0f, // left
     0.5f, -0.5f, 0.0f, // right
     0.0f,  0.5f, 0.0f, // top
  };
  */

  // Rectangle
  f32 verts[] = {
    // positions      
     0.5f,  0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    -0.5f,  0.5f, 0.0f,
  };
  unsigned int indices[] = {  
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
  };
  // Set up vertex attribute
  glGenVertexArrays(1, &prog->vao);
  // Bind VAO
  glBindVertexArray(prog->vao);
  // Set up vertex buffer object
  u32 vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(verts[0]), (void*)0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  // Set up EBO
  GLuint ebo;
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
  // Shader program
  size_t byte_count;
  // Vertx shader source
  const char* vertex_shader_source = read_file("shaders\\tri.vert", scratch, &byte_count);
  // Fragment shader source  
  const char* fragment_shader_source = read_file("shaders\\tri.frag", scratch, &byte_count);
  // Create vertex shader
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);
  // Create fragment shader
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
  glCompileShader(fragment_shader);
  // Create shader program
  prog->shader_program = glCreateProgram();
  glAttachShader(prog->shader_program, vertex_shader);
  glAttachShader(prog->shader_program, fragment_shader);
  glLinkProgram(prog->shader_program);
  // Clean up shaders
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

}


void cube_setup(arena *scratch, render_program *prog)
{
  /*
  f32 verts[] = {
    -0.5f, -0.5f, 0.0f, // left
     0.5f, -0.5f, 0.0f, // right
     0.0f,  0.5f, 0.0f, // top
  };
  */

  // position (x, y, z) + color (r, g, b)
  f32 verts[] = {
    // positions        // colors
    -1.0f,-1.0f,-1.0f,  1.0f, 0.0f, 0.0f, // 0 red
     1.0f,-1.0f,-1.0f,  0.0f, 1.0f, 0.0f, // 1 green
     1.0f, 1.0f,-1.0f,  0.0f, 0.0f, 1.0f, // 2 blue
    -1.0f, 1.0f,-1.0f,  1.0f, 1.0f, 0.0f, // 3 yellow
    -1.0f,-1.0f, 1.0f,  1.0f, 0.0f, 1.0f, // 4 magenta
     1.0f,-1.0f, 1.0f,  0.0f, 1.0f, 1.0f, // 5 indigo
     1.0f, 1.0f, 1.0f,  0.6f, 0.0f, 0.6f, // 6 violet
    -1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f  // 7 magenta
  };

  /*
  unsigned int indices[] = {
    0,1,2,  2,3,0, // front
    4,6,5,  6,4,7, // back
    0,3,7,  7,4,0, // left
    1,5,6,  6,2,1, // right
    3,2,6,  6,7,3, // top
    0,4,5,  5,1,0  // bottom
  };
  */

  uint32_t indices[] = {
    0,1, 1,2, 2,3, 3,0,        // bottom
    4,5, 5,6, 6,7, 7,4,        // top
    0,4, 1,5, 2,6, 3,7         // verticals
  };
  // Set up vertex attribute
  glGenVertexArrays(1, &prog->vao);
  // Bind VAO
  glBindVertexArray(prog->vao);
  // Set up vertex buffer object
  u32 vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

  // Just position
  // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(verts[0]), (void*)0);

  // position attribute (location = 0)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(f32), (void*)0);
  glEnableVertexAttribArray(0);

  // color attribute (location = 1)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(f32), (void*)(3 * sizeof(f32)));
  glEnableVertexAttribArray(1);


  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  // Set up EBO
  GLuint ebo;
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
  // Shader program
  size_t byte_count;
  // Vertx shader source
  const char* vertex_shader_source = read_file("shaders\\cube.vert", scratch, &byte_count);
  // Fragment shader source  
  const char* fragment_shader_source = read_file("shaders\\cube.frag", scratch, &byte_count);
  // Create vertex shader
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);
  // Create fragment shader
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
  glCompileShader(fragment_shader);
  // Create shader program
  prog->shader_program = glCreateProgram();
  ASSERT(prog->shader_program != 0, "ERROR: Failed to compile shaders.");
  glAttachShader(prog->shader_program, vertex_shader);
  glAttachShader(prog->shader_program, fragment_shader);
  glLinkProgram(prog->shader_program);
  ASSERT(prog->shader_program != 0, "ERROR: Failed to link shaders.");
  // Clean up shaders
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

}


void frame_init(render_state *state)
{
  // Clear screen
  glClearColor(0.01, 0.06, 0.06, 1.0f);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


void uniform_set(render_program *prog, f32 angle, f32 fov_deg, f32 aspect)
{
  glUseProgram(prog->shader_program);
  glm::mat4 model = glm::mat4(1.0f);
  glm::vec3 rotation_axis_norm = glm::vec3(0,1,0);
  model = glm::rotate(model, angle, rotation_axis_norm);
  // look at
  glm::vec3 camera_pos    = glm::vec3(0, 2.0f, 5.0f);
  glm::vec3 camera_target = glm::vec3(0,0,0);
  glm::vec3 camera_up     = glm::vec3(0,1,0);
  glm::mat4 view = glm::lookAt(camera_pos, camera_target, camera_up);
  // perspective
  f32 fov_rad = glm::radians(fov_deg);
  f32 znear = 0.1f;
  f32 zfar = 100.0f;
  glm::mat4 proj = glm::perspective(fov_rad, aspect, znear, zfar);
  glm::mat4 mvp = proj * view * model;
  GLint loc = glGetUniformLocation( prog->shader_program, "uMVP");
  glUniformMatrix4fv( loc, 1, GL_FALSE, &mvp[0][0]);
}


void draw_points(render_program *prog)
{
  // Bind our program
  glUseProgram(prog->shader_program);
  glBindVertexArray(prog->vao);
  // Draw the point
  glDrawArrays(GL_POINTS, 0, 1);
}


void draw_lines(render_program *prog)
{
  // Bind our program
  glUseProgram(prog->shader_program);
  // rotate
  glBindVertexArray(prog->vao);
  // Draw the point
  glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
}


void draw_triangles(render_program *prog)
{
  // Bind our program
  glUseProgram(prog->shader_program);
  f32 angle = 0.0f; // tweak speed here (radians per frame)
  float dist = 5.0f;                 // camera distance
  float fovY_deg = 45.0f;            // pick your FOV
  float width = 976.0f, height = 579.0f;
  float aspect   = width / height; // keep updated on resize
  glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0,1,0));
  glm::mat4 view = glm::lookAt(glm::vec3(0,2.0f,dist), glm::vec3(0,0.0f,0), glm::vec3(0,1,0));
  glm::mat4 proj = glm::perspective(glm::radians(fovY_deg), aspect, 0.1f, 100.0f);
  glm::mat4 mvp = proj * view * model;
  GLint loc = glGetUniformLocation( prog->shader_program, "uMVP");
  glUniformMatrix4fv( loc, 1, GL_FALSE, &mvp[0][0]);
  glBindVertexArray(prog->vao);
  // Draw the point
  // glDrawArrays(GL_TRIANGLES, 0, 3);
  // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}


void frame_render(render_state *state)
{
  SwapBuffers(state->context);
}


void render_close(render_state *state, render_program *prog)
{
  glDeleteVertexArrays(1, &prog->vao);
  // glDeleteBuffers(1, &prog->vbo);
  glDeleteProgram(prog->shader_program);
  HWND handle = WindowFromDC(state->context);
  ReleaseDC(handle, state->context);
}
