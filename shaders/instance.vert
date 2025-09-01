#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 aInstanceMatrix;

uniform mat4 uMVP;

void main()
{
  gl_Position = uMVP * aInstanceMatrix * vec4(aPos, 1.0f); 
}