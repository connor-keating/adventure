#pragma once

#include "core.h"
#include "linalg.h"


enum vertex_type
{
  VERTEX_UI,
  VERTEX_WORLD
};


struct entity
{
  u64 vert_start;
  u64 elem_start;
  u64 count;
};

union vertex1
{
  struct
  {
    fvec3 pos;
    fvec4 col;
  };
  f32 data[7];
};

