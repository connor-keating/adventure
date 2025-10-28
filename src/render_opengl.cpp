
#include "platform.h"
#include "opengl.h"

/*
YOU MUST USE SETUP A VBO BEFORE YOU CALL glVertexAttribPointer() 
OTHERWISE YOU'LL CRASH.
*/ 

struct render_state
{
  f32 width;
  f32 height;
};

struct render_buffer
{
  u32 vbo;
  u32 vao;
  u32 ebo;
};


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

// TODO: Remove window input, I can just set the size elsewhere
render_state render_init(platform_window *window)
{
  render_state state = {};
  platform_opengl_init();

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
  glViewport(0, 0, window->width, window->height);
  return state;
}


void frame_init(render_state *state)
{
  glViewport(0, 0, state->width, state->height);
  // Clear screen
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


void frame_render()
{
  platform_swapbuffers();
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


void render_point_size_set(f32 size)
{
  glPointSize(size);
}


u32 shader_compile(const char *filepath, i32 type, arena *scratch)
{
  size_t byte_count;
  const char* source = platform_file_read(filepath, scratch, &byte_count);
  // Shaders must be null-terminated.
  char *ending = arena_push_array(scratch, 1, char);
  *ending = '\0';
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


u32 texture3d_init(void *data, fvec3 shape)
{
  // Save current unpack alignment
  GLint prevUnpackAlign = 0;
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevUnpackAlign);
  // Set pixel alignment to 1 byte (default is 4 bytes)
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  // Generate texture ID
  u32 tex;
  glGenTextures( 1, &tex );
  glBindTexture( GL_TEXTURE_3D, tex );
  // Mutable texture
  glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, shape.x, shape.y, shape.z, 0, GL_RED, GL_UNSIGNED_BYTE, data);
  // Describe texture behavior
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
  // Unbind texture
  glBindTexture(GL_TEXTURE_3D, 0);
  // Restore previous unpack alignment
  glPixelStorei(GL_UNPACK_ALIGNMENT, prevUnpackAlign);
  return tex;
}


i32 texture3d_maxsize()
{
  // Check 3D texture limits
  i32 max3d;
  glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3d);
  return max3d;
}


void texture2d_bind(i32 slot, u32 texture_id)
{
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, texture_id);
}


void texture3d_bind(i32 slot, u32 texture_id)
{
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_3D, texture_id);
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


void draw_lines_instanced(render_buffer buffer, u32 shader_program, u32 count)
{
  // Bind our program
  glUseProgram(shader_program);
  glBindVertexArray(buffer.vao);
  glDrawElementsInstanced(
    GL_LINES,// 	GLenum mode,
    24, // GLsizei count,
    GL_UNSIGNED_INT, // GLenum type,
    0, // const void * indices, // Bound in EBO
    count // GLsizei primcount
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


void draw_wireframe_elements(render_buffer buffer, u32 shader_program, u32 amount, void *starting_offset)
{
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  // Bind our program
  glUseProgram(shader_program);
  glBindVertexArray(buffer.vao);
  // Draw the point
  glDrawElements(GL_TRIANGLES, amount, GL_UNSIGNED_INT, starting_offset);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


void shader_close(u32 shader_program)
{
  glDeleteProgram(shader_program);
}

void render_vsync(i32 status)
{
  wglSwapIntervalEXT(status); // 1 is on 0 is off.
}
