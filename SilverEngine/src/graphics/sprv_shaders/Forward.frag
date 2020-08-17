#version 450

#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 FragNormal;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform Material {
	vec4 diffuseColor;
};

void main() {
	
	outColor = vec4(FragNormal, 1.f);

}