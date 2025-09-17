#version 430 core

in vec4 color;
in vec2 texCoord;

uniform sampler2D texture_image;

out vec4 fragColor;

void main()
{
  fragColor = vec4(texture(texture_image, texCoord).r) * color;
}
