#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>


void text_atlas_init()
{
  const char *font_file = "C:\\WINDOWS\\Fonts\\arial.ttf";
  int answer = file_exists(font_file);
  ASSERT(answer == 1, "Font file not found.");
  printf("Done\n");
}
