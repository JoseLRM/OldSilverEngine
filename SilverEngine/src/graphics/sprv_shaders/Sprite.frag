#version 450

// Vertex Shader Input
layout (location = 0) in vec4 FragColor;
layout (location = 1) in vec2 FragTexCoord;

// Output
layout (location = 0) out vec4 outColor;

void main()
{
	outColor = FragColor;
}