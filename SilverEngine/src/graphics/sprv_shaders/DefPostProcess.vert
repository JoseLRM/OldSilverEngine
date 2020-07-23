#version 450

layout(location = 0) in vec2 Position;

layout(location = 0) out vec2 texCoord;

void main()
{
	gl_Position = vec4(Position, 0.f, 1.f);
	texCoord = (Position / 2.f) + 0.5f;
}