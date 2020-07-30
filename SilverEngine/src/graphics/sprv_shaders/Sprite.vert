#version 450 

layout (location = 0) in vec4 Position;
layout (location = 1) in vec2 TexCoord;
layout (location = 2) in vec4 Color;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec2 FragTexCoord;

void main()
{
	FragColor = Color;
	FragTexCoord = TexCoord;

	gl_Position = Position;
}