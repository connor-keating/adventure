#version 430 core

flat in uint id;
out vec4 FragColor;

uniform vec3 mycolor;

// Texture3D
uniform sampler3D voxels;
uniform vec3 dimension;


void main() 
{
  int x = int(id % uint(dimension.x));
  int y = int((id / uint(dimension.x)) % uint(dimension.y));
  int z = int(id / uint(dimension.x * dimension.y));

  // Read from 3D texture to determine if this voxel is "on"
  float is_on = texelFetch(voxels, ivec3(x,y,z), 0).r;
  if (is_on < 1.0f) discard;

  // Color each cube based on its 3D grid position
  vec3 color = vec3(float(x), float(y), float(z)) / vec3(dimension - 1.0);
  FragColor = vec4(color, is_on);
}
