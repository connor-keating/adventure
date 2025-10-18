#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include "core.h"
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


typedef f32 fmat4[4][4];

typedef struct model_view_projection model_view_projection;
struct model_view_projection
{
    fmat4 model;
    fmat4 view;
    fmat4 proj;
};


fvec3 fvec3_add(fvec3 a, fvec3 b);
fvec3 fvec3_sub(fvec3 a, fvec3 b);
fvec3 fvec3_scale(fvec3 vec, f32 scalar);
void fmat4_identity(fmat4 mat);
void fmat4_rotate(fmat4 out, f32 angle_rad, fvec3 axis);
void fmat4_perspective(fmat4 out, f32 fov_rad, f32 aspect, f32 znear, f32 zfar);
void fmat4_lookat(fmat4 out, fvec3 eye, fvec3 center, fvec3 up);
// void fmat4_lookat_cmaj(fmat4 m, fvec3 eye, fvec3 center, fvec3 up);


inline fvec2 fvec2_init(f32 x, f32 y)
{
  fvec2 v = { .array = {x, y} };
  return v;
}


ivec3 ivec3_uniform(i32 value)
{
  return ivec3{ {value, value, value} };
}


fvec3 fvec3_uniform(f32 value)
{
  return fvec3{ {value, value, value} };
}


fvec3 fvec3_add(fvec3 a, fvec3 b)
{
  fvec3 out;
  out.x = a.x + b.x;
  out.y = a.y + b.y;
  out.z = a.z + b.z;
  return out;
}


fvec2 fvec2_sub(fvec2 a, fvec2 b)
{
  fvec2 out = {};
  out.x = a.x - b.x;
  out.y = a.y - b.y;
  return out;
}


fvec3 fvec3_sub(fvec3 a, fvec3 b)
{
  fvec3 out = {};
  out.x = a.x - b.x;
  out.y = a.y - b.y;
  out.z = a.z - b.z;
  return out;
}


fvec3 fvec3_div(fvec3 a, f32 scalar)
{
  a.x /= scalar;
  a.y /= scalar;
  a.z /= scalar;
  return a;
}


fvec2 fvec2_max(fvec2 a, fvec2 b)
{
  fvec2 out;
  out.x = (a.x >= b.x) ? a.x : b.x;
  out.y = (a.y >= b.y) ? a.y : b.y;
  return out;
}


fvec3 fvec3_max(fvec3 a, fvec3 b)
{
  fvec3 out;
  out.x = (a.x >= b.x) ? a.x : b.x;
  out.y = (a.y >= b.y) ? a.y : b.y;
  out.z = (a.z >= b.z) ? a.z : b.z;
  return out;
}


f32 fvec3_max_elem(fvec3 a)
{
  f32 max = 0;
  max = (a.x > a.y) ? a.x : a.y;
  max = (max > a.z) ? max : a.z;
  return max;
}


fvec2 fvec2_min(fvec2 a, fvec2 b)
{
  fvec2 out;
  out.x = (a.x <= b.x) ? a.x : b.x;
  out.y = (a.y <= b.y) ? a.y : b.y;
  return out;
}


fvec3 fvec3_min(fvec3 a, fvec3 b)
{
  fvec3 out;
  out.x = (a.x <= b.x) ? a.x : b.x;
  out.y = (a.y <= b.y) ? a.y : b.y;
  out.z = (a.z <= b.z) ? a.z : b.z;
  return out;
}


fvec2 fvec2_scale(fvec2 vec, f32 scalar)
{
  vec.x *= scalar;
  vec.y *= scalar;
  return vec;
}


fvec3 fvec3_scale(fvec3 vec, f32 scalar)
{
  vec.x *= scalar;
  vec.y *= scalar;
  vec.z *= scalar;
  return vec;
}


inline f32 dot2(fvec2 a, fvec2 b)
{
  f32 product = (a.x*b.x) + (a.y*b.y);
  return product;
}


inline f32 dot3(fvec3 a, fvec3 b)
{
  f32 product = (a.x*b.x) + (a.y*b.y) + (a.z*b.z);
  return product;
}


inline fvec3 normalize3(fvec3 vec)
{
  f32 vec_length = sqrtf(dot3(vec, vec));
  if (vec_length > 0.0f)
  {
    f32 inverse = 1.0f / vec_length;
    vec = fvec3_scale(vec, inverse);
  }
  return vec;
}


inline f32 cross2(fvec2 a, fvec2 b)
{
  f32 out = (a.x * b.y) - (b.x * a.y);
  return out;
}


inline fvec3 cross3(fvec3 a, fvec3 b)
{
  fvec3 out = {};
  out.x = (a.y*b.z) - (a.z*b.y);
  out.y = (a.z*b.x) - (a.x*b.z);
  out.z = (a.x*b.y) - (a.y*b.x);
  return out;
}


void fmat4_identity(fmat4 mat)
{
  i8 i, j, dim;
  dim = 4;
	for(i=0; i<dim; ++i)
  {
    for(j=0; j<dim; ++j)
    {
      mat[i][j] = i==j ? 1.f : 0.f;
    }
  }
}


inline void fmat4_col_scale(fmat4 mat, u8 index, f32 scalar)
{
  mat[0][index] *= scalar;
  mat[1][index] *= scalar;
  mat[2][index] *= scalar;
  mat[3][index] *= scalar;
}


inline void matmul4(fmat4 out, fmat4 a, fmat4 b)
{
  for (u8 row = 0; row < 4; ++row)
  {
    for (u8 col = 0; col < 4; ++col)
    {
      for (u8 index = 0; index < 4; ++index)
      {
        out[row][col] += a[row][index] * b[index][col];
      }
    }
  }
}


void fmat4_rotate(fmat4 out, f32 angle_rad, fvec3 axis)
{
  fmat4 final = {};

  axis = normalize3(axis);  // Ensure axis is normalized

  f32 c = cosf(angle_rad);
  f32 s = sinf(angle_rad);
  f32 one_c = 1.0f - c;
  fvec3 temp = axis;
  temp = fvec3_scale(temp, one_c);

  fmat4 rotate;
  rotate[0][0] = c + temp.x * axis.x;
  rotate[0][1] = temp.x * axis.y + s * axis.z;
  rotate[0][2] = temp.x * axis.z - s * axis.y;
  rotate[1][0] = temp.y * axis.x - s * axis.z;
  rotate[1][1] = c + temp.y * axis.y;
  rotate[1][2] = temp.y * axis.z + s * axis.x;
  rotate[2][0] = temp.z * axis.x + s * axis.y;
  rotate[2][1] = temp.z * axis.y - s * axis.x;
  rotate[2][2] = c + temp.z * axis.z;
  rotate[0][3] = 0.0f;
  rotate[1][3] = 0.0f;
  rotate[2][3] = 0.0f;
  rotate[3][3] = 1.0f;
  matmul4(final, out, rotate);
  memcpy(out, final, sizeof(fmat4));
}


void fmat4_perspective(fmat4 out, f32 fov_rad, f32 aspect, f32 znear, f32 zfar)
{
  f32 tan_half_fov = tanf(fov_rad / 2.0f);
  memset(out, 0, sizeof(fmat4));
  out[0][0] = 1.0f / (aspect * tan_half_fov);
  out[1][1] = 1.0f / tan_half_fov;
  out[2][2] = -(zfar + znear) / (zfar - znear);
  out[2][3] = -1.0f;
  out[3][2] = -(2.0f * zfar * znear) / (zfar - znear);
}


void fmat4_lookat(fmat4 out, fvec3 eye, fvec3 center, fvec3 up)
{
  fvec3 f, s, u;

  f = normalize3(fvec3_sub(center, eye));
  s = normalize3(cross3(f, up));
  u = cross3(s, f);

  fmat4_identity(out);
  out[0][0] = s.x;
  out[1][0] = s.y;
  out[2][0] = s.z;
  out[0][1] = u.x;
  out[1][1] = u.y;
  out[2][1] = u.z;
  out[0][2] = -f.x;
  out[1][2] = -f.y;
  out[2][2] = -f.z;
  out[3][0] = -dot3(s, eye);
  out[3][1] = -dot3(u, eye);
  out[3][2] =  dot3(f, eye);
}
