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