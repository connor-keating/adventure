#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 3) in mat4 aInstanceMatrix;

out vec3 vColor;

uniform mat4 uMVP;

void main()
{
  vColor = aColor;
  gl_Position = uMVP * aInstanceMatrix * vec4(aPos, 1.0f); 
}