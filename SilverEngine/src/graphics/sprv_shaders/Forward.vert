#version 460

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoord;

layout (location = 3) in mat4 ModelViewMatrix;

layout (set = 0, binding = 0) uniform camera {
	mat4 pm;
};

void main() {
	
	gl_Position = pm * ModelViewMatrix * vec4(Position, 1.f);

}