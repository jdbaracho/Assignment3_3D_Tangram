#version 330 core

in vec3 exPosition;
in vec2 exTexcoord;
in vec3 exNormal;

out vec4 FragmentColor;

uniform vec3 Color;

void main(void)
{
  	vec3 color = (exNormal * 0.1f + Color);
	FragmentColor = vec4(color,1.0);
}
