#version 430 core

layout (location = 0) in vec3 aPos;

uniform mat4 proj;

void main()
{
  gl_Position = proj * vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
