#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>


struct char_vertex
{
  glm::vec3 position;
  glm::vec4 color;
  glm::vec2 texCoord;
};


void text_atlas_init(arena *memory)
{
  // Intialize font atlas parameters
  u32 char_start   = 32;
  u32 char_count   = 95;
  u32 atlas_width  = 512;
  u32 atlas_height = 512;
  f32 char_size    = 64.0f;
  glm::mat4 view_projection;
  stbtt_packedchar chars_packed[95];
  stbtt_aligned_quad quads_aligned[95];
  u32 pixel_height;
  // Check file exists
  const char *font_file = "C:\\WINDOWS\\Fonts\\arial.ttf";
  int answer = file_exists(font_file);
  ASSERT(answer == 1, "Font file not found.");
  // Read the font file
  size_t font_file_size;
  u8 *font_file_contents = (u8*) read_file(font_file, memory, &font_file_size);
  u32 font_count = stbtt_GetNumberOfFonts(font_file_contents);
  printf("File font count: %d\n", font_count);
  // Load the font
  stbtt_fontinfo font_info = {};
  i32 success = stbtt_InitFont(&font_info, font_file_contents, 0);
  ASSERT(success != 0, "Font init failed!");
  // Load the font atlas texture
  size_t atlas_size = (size_t) (atlas_width * atlas_height);
  u8 *atlas_texture = arena_alloc_array(memory, atlas_size, u8);
  stbtt_pack_context ctx = {};
  stbtt_PackBegin(
    &ctx,           // Initialize the context
    atlas_texture,  // Font atlas texture data
    atlas_width,    // Width of atlas texture
    atlas_height,   // Height of atlas texture
    0,              // Stride in bytes
    1,              // Padding between glyphs
    nullptr
  );
  stbtt_PackFontRange(
    &ctx,
    font_file_contents,
    0,
    char_size,
    char_start,
    char_count,
    chars_packed
  );
  stbtt_PackEnd(&ctx);
  stbi_write_png("assets\\font-atlas.png", atlas_width, atlas_height, 1, atlas_texture, atlas_width);
  printf("Done\n");
}
