#version 430 core

out vec4 FragColor;

uniform vec3 mycolor;


void main() 
{
  FragColor = vec4(mycolor, 1.0);
}
