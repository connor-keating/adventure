#include <windows.h>
#include <gl/GL.h>

/*
YOU MUST USE SETUP A VBO BEFORE YOU CALL glVertexAttribPointer() 
OTHERWISE YOU'LL CRASH.
*/ 

struct render_state
{
  HDC context;
  f32 width;
  f32 height;
};

struct render_buffer
{
  u32 vbo;
  u32 vao;
  u32 ebo;
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
#define GL_R8                             0x8229
#define GL_SHADER_STORAGE_BUFFER          0x90D2
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
    GL_FUNC_SIGNATURE(void, glDebugMessageInsert, GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char *message)              \
    GL_FUNC_SIGNATURE(void, glVertexAttribDivisor, GLuint index, GLuint divisor)                                                                            \
    GL_FUNC_SIGNATURE(void, glDrawElementsInstanced, GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei primcount)                      \
    GL_FUNC_SIGNATURE(void, glBindBufferBase,	GLenum target, GLuint index, GLuint buffer)                      \



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
    // ASSERT(false, sentence_buffer);
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
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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


render_buffer render_buffer_init(void *data, size_t length)
{
  render_buffer buffer = {};
  // Set up Vertex Buffer Object
  glGenBuffers(1, &buffer.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
  glBufferData(GL_ARRAY_BUFFER, length, data, GL_STATIC_DRAW);
  // Set up vertex attribute
  glGenVertexArrays(1, &buffer.vao);
  return buffer;
}


render_buffer render_buffer_dynamic_init(void *data, size_t length)
{
  render_buffer buffer = {};
  // Set up Vertex Buffer Object
  glGenBuffers(1, &buffer.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
  glBufferData(GL_ARRAY_BUFFER, length, data, GL_DYNAMIC_DRAW);
  // Set up vertex attribute
  glGenVertexArrays(1, &buffer.vao);
  return buffer;
}


void render_buffer_elements_init(render_buffer *buffer, const void *indices, size_t byte_size)
{
  glBindVertexArray(buffer->vao);
  glGenBuffers(1, &buffer->ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, byte_size, indices, GL_STATIC_DRAW);
  glBindVertexArray(0);
}


void render_buffer_attribute(render_buffer buffer, u32 index, u32 element_count, size_t stride, void *offset)
{
  // Bind the VAO and VBO
  glBindVertexArray(buffer.vao);
  glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
  // Describe it
  glVertexAttribPointer(index, element_count, GL_FLOAT, GL_FALSE, stride, offset);
  // Turn it on
  glEnableVertexAttribArray(index);
  /*
  Example:
  index = 0;
  size = 3;
  type = GL_FLOAT;
  normalize = GL_FALSE;
  stride = sizeof(vertex_array);
  starting_offset = (void*)0;
  */
}


void render_buffer_push(render_buffer buffer, void* data, i32 start_offset, size_t byte_count)
{
  glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
  glBufferSubData(GL_ARRAY_BUFFER, start_offset, byte_count, data);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void render_buffer_elements_push(render_buffer buffer, void* data, i64 starting_byte_offset, size_t byte_count)
{
  // start_offset == starting address in GPU buffer.
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.ebo);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, starting_byte_offset, byte_count, data);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


u32 shader_compile(const char *filepath, i32 type, arena *scratch)
{
  size_t byte_count;
  const char* source = read_textfile(filepath, scratch, &byte_count);
  u32 shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  return shader;
}


void shader_storage_init(u32 binding_index, void *data, size_t byte_count)
{
  GLuint ssbo = 0;
  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, byte_count, data, GL_DYNAMIC_DRAW);
  // Bind whole buffer to binding point 1 (matches shader)
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_index, ssbo);
  // unbind
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}


u32 render_program_init(arena *scratch, const char *vert_shader, const char *frag_shader)
{
  // Init output
  u32 program;
  // Set up shaders 
  u32 vertex_shader   = shader_compile(vert_shader, GL_VERTEX_SHADER, scratch);
  u32 fragment_shader = shader_compile(frag_shader, GL_FRAGMENT_SHADER, scratch);
  // Create shader program
  program = glCreateProgram();
  ASSERT(program != 0, "ERROR: Failed to create shader program.");
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  {
    int success = 0;
    char log[512] = {0};
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(program, 512, 0, log);
        ASSERT(success, log);
    }
  }
  // Clean up shaders
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  return program;
}


void ui_init(render_state *state, arena *scratch, render_buffer buffer)
{
  /*
  // The quad vertices
  f32 verts[] = {
    // Position          UV coords
    -1.0f, -1.0f, 0.0f, //  0.0f, 0.0f, // Lower left
     1.0f, -1.0f, 0.0f, //  1.0f, 0.0f, // Lower right
    -1.0f,  1.0f, 0.0f, //  0.0f, 1.0f, // Upper left
     1.0f,  1.0f, 0.0f, //  1.0f, 1.0f, // Upper right
  };
  */
  
  // Bind VAO
  glBindVertexArray(buffer.vao);

  // position attribute (location = 0)
  size_t vert_size = 3 * sizeof(f32);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vert_size, (void*)0);
  glEnableVertexAttribArray(0);

  // texture attribute (location = 1)
  // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vert_size, (void*)(3 * sizeof(f32)));
  // glEnableVertexAttribArray(1);

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void render_text_init(render_buffer buffer)
{
  // enable position
  // render_buffer_attribute(0, 3, 9 * sizeof(f32), 0);
}


u32 texture2d_1channel_init(void *image, u32 width, u32 height)
{
  u32 texture_id;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  // Create texture image on GPU
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_R8,
    width,
    height,
    0,
    GL_RED,
    GL_UNSIGNED_BYTE,
    image
  );
  // Use linear filter when sampling
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // What happens if texture coords go outside [0, 1] (GL_REPEAT = Tile texture)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // Unbind texture
  glBindTexture(GL_TEXTURE_2D, 0);
  return texture_id;
}


void texture_bind(i32 slot, u32 texture_id)
{
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, texture_id);
}


void uniform_set_i32(u32 shader_program, const char *name, i32 data)
{
  glUseProgram(shader_program);
  i32 location = glGetUniformLocation(shader_program, name);
  glUniform1i(location, data);
}


void buffer_tri_add()
{
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
}


void buffer_cube_add()
{
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
  u32 indices[] = {
    0,1, 1,2, 2,3, 3,0,        // bottom
    4,5, 5,6, 6,7, 7,4,        // top
    0,4, 1,5, 2,6, 3,7         // verticals
  };
}


render_buffer instance_setup(arena *scratch)
{
  f32 verts[] = {
    // positions      
    -1.0f,-1.0f,-1.0f,
     1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
  };
  u32 indices[] = {
    0,1, 1,2, 2,3, 3,0,        // bottom
    4,5, 5,6, 6,7, 7,4,        // top
    0,4, 1,5, 2,6, 3,7         // verticals
  };

  render_buffer buffer = render_buffer_init((void*)verts, sizeof(verts));
  render_buffer_attribute(buffer, 0, 3, 3*sizeof(f32), (void*)0);
  // Set up EBO
  render_buffer_elements_init(&buffer, indices, sizeof(indices));

  // Create instance transforms
  u32 cube_count = 2;
  glm::mat4 *modelmats = arena_push_array(scratch, cube_count, glm::mat4); 
  for (int i = 0; i < cube_count; i++)
  {
    // Transformations are applied in reverse multiplication order.
    glm::mat4 transform = glm::mat4(1.0f);
    f32 x_offset = ( i==0 ) ? -0.5 : 0.5f;
    transform = glm::translate(transform, glm::vec3(x_offset, 0.0f, 0.0f));
    transform = glm::scale(transform, glm::vec3(0.5f, 1.0f, 1.0f));
    modelmats[i] = transform;
  }

  // Keep transforms in storage buffer
  size_t transform_buffer_size = cube_count * sizeof(glm::mat4);
  shader_storage_init(0, (void*)&modelmats[0], transform_buffer_size);

  return buffer;
}


void frame_init(render_state *state)
{
  glViewport(0, 0, state->width, state->height);
  // Clear screen
  glClearColor(0.01, 0.06, 0.06, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


void uniform_set_mat4(u32 shader_program, const char *name, const f32 *data)
{
  glUseProgram(shader_program);
  GLint loc = glGetUniformLocation( shader_program, name);
  glUniformMatrix4fv( loc, 1, GL_FALSE, data);
}


void uniform_set_vec3(u32 shader_program, const char *name, fvec3 data)
{
  glUseProgram(shader_program);
  GLint loc = glGetUniformLocation( shader_program, name);
  glUniform3fv( loc, 1, data.array);
}


void uniform_set(u32 shader_program, f32 angle, f32 fov_deg, f32 aspect)
{
  glUseProgram(shader_program);
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
  GLint loc = glGetUniformLocation( shader_program, "uMVP");
  glUniformMatrix4fv( loc, 1, GL_FALSE, &mvp[0][0]);
}


void draw_text(render_buffer buffer, u32 shader_program, u32 count)
{
  // Disable depth writing for transparent text
  glDepthMask(GL_FALSE);

  // Input the vertex data
  glBindVertexArray(buffer.vao);
  glDrawArrays(GL_TRIANGLES, 0, count);

  // Re-enable depth writing
  glDepthMask(GL_TRUE);
}


void draw_ui(render_buffer buffer, u32 shader_program)
{
  glUseProgram(shader_program);
  glBindVertexArray(buffer.vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void draw_lines_instanced(render_buffer buffer, u32 shader_program)
{
  // Bind our program
  glUseProgram(shader_program);
  glBindVertexArray(buffer.vao);
  glDrawElementsInstanced(
    GL_LINES,// 	GLenum mode,
    24, // GLsizei count,
    GL_UNSIGNED_INT, // GLenum type,
    0, // const void * indices, // Bound in EBO
    2 // GLsizei primcount
  );
}


void draw_points(render_buffer buffer, u32 shader_program, u32 amount)
{
  u32 index_start = 0;
  // Bind our program
  glUseProgram(shader_program);
  glBindVertexArray(buffer.vao);
  // Draw the point
  glDrawArrays(GL_POINTS, index_start, amount);
}


void draw_lines(render_buffer buffer, u32 shader, u32 amount)
{
  glUseProgram(shader);
  glBindVertexArray(buffer.vao);
  glDrawArrays(GL_LINES, 0, amount);
}


void draw_lines_elements(render_buffer buffer, u32 shader_program, u32 amount, void *starting_offset)
{
  // Bind our program
  glUseProgram(shader_program);
  glBindVertexArray(buffer.vao);
  // Draw the point
  glDrawElements(GL_LINES, amount, GL_UNSIGNED_INT, starting_offset);
}


void frame_render(render_state *state)
{
  SwapBuffers(state->context);
}


void shader_close(u32 shader_program)
{
  glDeleteProgram(shader_program);
}


void render_close(render_state *state)
{
  HWND handle = WindowFromDC(state->context);
  ReleaseDC(handle, state->context);
}
