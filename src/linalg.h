#pragma once

#include "core.h"


#ifdef _D3D
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#define PI 	3.14159265358979323846f
#define DegreesToRadians(degrees) (degrees*(PI/180.0f))
#define RadiansToDegrees(radians) (radians(180.0f/PI))


typedef union fvec2 fvec2;
union fvec2
{
  struct 
  {
    f32 x, y;
  };
  f32 array[2];
};


union ivec2
{
  struct
  {
    i32 x, y;
  };
  i32 array[2];
};

typedef union fvec3 fvec3;
union fvec3
{
  struct 
  {
    f32 x, y, z;
  };
  f32 array[3];
};


union ivec3
{
  struct
  {
    i32 x, y, z;
  };
  i32 array[3];
};


union fvec4
{
  struct 
  {
    f32 x, y, z, w;
  };
  f32 array[4];
};

typedef f32 fmat4[4][4];


typedef struct model_view_projection model_view_projection;
struct model_view_projection
{
    fmat4 model;
    fmat4 view;
    fmat4 proj;
};



fvec2 fvec2_init(f32 x, f32 y);
fvec3 fvec3_init(f32 x, f32 y, f32 z);
fvec4 fvec4_init(f32 x, f32 y, f32 z, f32 w);
fvec3 fvec3_add(fvec3 a, fvec3 b);
fvec3 fvec3_sub(fvec3 a, fvec3 b);
fvec3 fvec3_scale(fvec3 vec, f32 scalar);
void fmat4_identity(fmat4 mat);
void fmat4_rotate(fmat4 out, f32 angle_rad, fvec3 axis);
void fmat4_perspective(fmat4 out, f32 fov_rad, f32 aspect, f32 znear, f32 zfar);
void fmat4_lookat(fmat4 out, fvec3 eye, fvec3 center, fvec3 up);
// void fmat4_lookat_cmaj(fmat4 m, fvec3 eye, fvec3 center, fvec3 up);