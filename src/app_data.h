#pragma once


union vertex1
{
  struct
  {
    fvec3 pos;
    fvec4 col;
    fvec2 tex;
  };
  f32 data[9];
};


struct camera
{
  glm::mat4 view_inv;            // 64 bytes
  glm::mat4 proj_inv;            // 64 bytes
  glm::mat4 wrld_inv;            // 64 bytes - inverse rotation for volume
  glm::vec3 pos;                 // 12 bytes
  f32 _padding;                  // 4 bytes → pad to 16-byte multiple
};