#include "render.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>


// How many ANSI characters to load from the start (32 - 127)
#define CHAR_START 32 
#define CHAR_COUNT 95 


struct char_vertex
{
  glm::vec3 position;
  glm::vec4 color;
  glm::vec2 texCoord;
};


struct char_atlas
{
  u8 *image;
  u32 width     = 512;
  u32 height    = 512;
  f32 char_size = 64.0f;
};


// Globals
stbtt_packedchar chars_packed[CHAR_COUNT];
stbtt_aligned_quad quads_aligned[CHAR_COUNT];



arena text_buffer_init(arena *parent, u32 vertex_count)
{
  arena a = subarena_for(parent, vertex_count, char_vertex);
  return a;
}


char_atlas text_atlas_init(arena *memory, const char * font_file)
{
  // Intialize font atlas output
  char_atlas atlas = {};
  glm::mat4 view_projection;
  u32 pixel_height;
  // Check file exists
  int answer = platform_file_exists(font_file);
  ASSERT(answer == 1, "Font file not found.");
  // Read the font file
  size_t font_file_size;
  u8 *font_file_contents = (u8*) platform_file_read(font_file, memory, &font_file_size);
  u32 font_count = stbtt_GetNumberOfFonts(font_file_contents);
  ASSERT(font_count == 1, "Found more than one font in the file.");
  // Load the font
  stbtt_fontinfo font_info = {};
  i32 success = stbtt_InitFont(&font_info, font_file_contents, 0);
  ASSERT(success != 0, "Font init failed!");
  // Load the font atlas texture
  size_t atlas_size = (size_t) (atlas.width * atlas.height);
  atlas.image = arena_push_array(memory, atlas_size, u8);
  stbtt_pack_context ctx = {};
  stbtt_PackBegin(
    &ctx,           // Initialize the context
    atlas.image,    // Font atlas texture data
    atlas.width,    // Width of atlas texture
    atlas.height,   // Height of atlas texture
    0,              // Stride in bytes
    1,              // Padding between glyphs
    nullptr
  );
  stbtt_PackFontRange(
    &ctx,
    font_file_contents,
    0,
    atlas.char_size,
    CHAR_START,
    CHAR_COUNT,
    chars_packed
  );
  stbtt_PackEnd(&ctx);
  // DEBUGGING: 
  // stbi_write_png("assets\\font-atlas.png", atlas.width, atlas.height, 1, atlas.image, atlas.width);
  // Create 3D API vertices for chars
  for (i32 i = 0; i < CHAR_COUNT; ++i)
  {
    f32 screen_pos_x, screen_pos_y;
    stbtt_GetPackedQuad(
      chars_packed,
      atlas.width,
      atlas.height,
      i,
      &screen_pos_x,
      &screen_pos_y,
      &quads_aligned[i],
      0
    );
  }
  return atlas;
}


texture_ptr text_init(arena *memory, const char * font_file)
{
  // Create the char atlas bitmap image
  char_atlas atlas = text_atlas_init(memory, font_file);
  // Upload it to GPU texture
  texture_ptr font_texture = texture2d_init(memory, atlas.image, atlas.width, atlas.height, 1);
  return font_texture;
}



rbuffer_ptr text_gpu_init(arena *a, void *cpu_buffer, u32 vert_count)
{
  // Render stuff
  size_t text_vert_max = vert_count * sizeof(char_vertex); 
  rbuffer_ptr buffer = render_buffer_dynamic_init(
    a,
    VERTS,
    cpu_buffer,
    sizeof(f32)*9,
    text_vert_max
  );
  // TODO: Fix OpenGL implementation.
  /*
  render_buffer buffer = render_buffer_dynamic_init(nullptr, text_vert_max);
  char_vertex example = {};
  u32 text_stride = sizeof(example);
  // TODO: Can I get this from the parameters themselves?
  u32 pos_elem_count = 3;
  u32 col_elem_count = 4;
  u32 tex_elem_count = 2;
  void *pos_offset = 0;
  void *col_offset = (void*) (pos_elem_count * sizeof(f32));
  void *tex_offset = (void*) ( (pos_elem_count+col_elem_count) * sizeof(f32) );
  render_buffer_attribute(buffer, 0, pos_elem_count, text_stride, pos_offset);
  render_buffer_attribute(buffer, 1, col_elem_count, text_stride, col_offset);
  render_buffer_attribute(buffer, 2, tex_elem_count, text_stride, tex_offset);
  */
  return buffer;
}


void text_add(arena *a, const char *text, u32 length, i32 window_height, glm::vec3 position, f32 size, glm::vec4 color, f32 pixel_scale)
{
  i32 order[6];
  // If Counterclockwise (OpenGL default, D3D11 must set in rasterizer)
  // Tri 1
  order[0] = 0;
  order[1] = 1;
  order[2] = 2;
  // Tri 2
  order[3] = 0;
  order[4] = 2;
  order[5] = 3;
  glm::vec3 pos_local = position;
  for (i32 i = 0; i < length; ++i)
  {
    // TODO: if check: Is the char inside our 32-127?
    // check if text length will overflow the buffer
    char ch = text[i];
    u32 char_index = ch - CHAR_START;
    stbtt_packedchar char_pack = chars_packed[char_index];
    stbtt_aligned_quad quad = quads_aligned[char_index];
    glm::vec2 glyph_size = {
      (char_pack.x1 - char_pack.x0) * pixel_scale * size,
      (char_pack.y1 - char_pack.y0) * pixel_scale * size,
    };
    glm::vec2 glyph_bounds_sw = {
      pos_local.x + (char_pack.xoff * pixel_scale * size),
      pos_local.y + (char_pack.yoff + char_pack.y1 - char_pack.y0) * pixel_scale * size
    };
    // The order of vertices of a quad goes top-left, top-right, bottom-left, bottom-right
    glm::vec2 glyph_vertices[4]; 
    glm::vec2 glyph_texture[4];
    // Verts
    glyph_vertices[0] = { glyph_bounds_sw.x + glyph_size.x, glyph_bounds_sw.y + glyph_size.y };
    glyph_vertices[1] = { glyph_bounds_sw.x, glyph_bounds_sw.y + glyph_size.y };
    glyph_vertices[2] = { glyph_bounds_sw.x, glyph_bounds_sw.y };
    glyph_vertices[3] = { glyph_bounds_sw.x + glyph_size.x, glyph_bounds_sw.y };
    // Texture 
    glyph_texture[0] = { quad.s1, quad.t0 };
    glyph_texture[1] = { quad.s0, quad.t0 };
    glyph_texture[2] = { quad.s0, quad.t1 };
    glyph_texture[3] = { quad.s1, quad.t1 };
    // We need to fill the vertex buffer by 6 vertices to render a quad as we are rendering a quad as 2 triangles
    // The order used is in the 'order' array
    // order = [0, 1, 2, 0, 2, 3] is meant to represent 2 triangles: 
    // one by glyph_vertices[0], glyph_vertices[1], glyph_vertices[2] and one by glyph_vertices[0], glyph_vertices[2], glyph_vertices[3]
    for(int i = 0; i < 6; i++)
    {
      char_vertex *element = arena_push_struct(a, char_vertex);
      element->position = glm::vec3(glyph_vertices[order[i]], position.z);
      element->color = color;
      element->texCoord = glyph_texture[order[i]];
    }
    // Update the position to render the next glyph specified by packedChar.xadvance.
    pos_local.x += char_pack.xadvance * pixel_scale * size;
    // TODO: else if char is newline advance it differently
  }
}



u32 text_count(arena *a)
{
  u32 vertex_count = (u32) a->offset_new / sizeof(char_vertex);
  return vertex_count;
}


#if 0

glm::mat4 text_projection(f32 aspect_ratio)
{
  glm::mat4 projectionMat = glm::ortho(-aspect_ratio, aspect_ratio, -1.0f, 1.0f);
  glm::mat4 viewMat = glm::mat4(1.0f);
  viewMat = glm::translate(viewMat, {0.0f, 0.0f, 0.0f});
  viewMat = glm::rotate(viewMat, 0.0f, {1, 0, 0});
  viewMat = glm::rotate(viewMat, 0.0f, {0, 1, 0});
  viewMat = glm::rotate(viewMat, 0.0f, {0, 0, 1});
  viewMat = glm::scale(viewMat, {1.0f, 1.0f, 1.0f});
  return projectionMat;
}

#endif