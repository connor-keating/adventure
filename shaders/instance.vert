#version 430 core
layout (location = 0) in vec3 aPos;

layout (std430, binding = 0) buffer InstanceData {
  mat4 M[];            // unbounded array of instance model matrices
} inst;

uniform mat4 uMVP;

void main()
{
  mat4 model = inst.M[gl_InstanceID];
  gl_Position = uMVP * model * vec4(aPos, 1.0f); 
}