#version 450

layout(location = 0) in vec2 Position;
layout(location = 1) in vec3 Color;

layout(location = 0) out vec3 outColor;

layout(binding = 0) uniform Global {
	
	mat4 matrix;

};

void main()
{
	gl_Position = vec4(Position, 0.f, 1.f) * matrix;

	outColor = Color;
}