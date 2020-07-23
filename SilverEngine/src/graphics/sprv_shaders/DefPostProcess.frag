#version 450

layout(binding = 0) uniform sampler sam;
layout(binding = 1) uniform texture2D tex;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(sampler2D(tex, sam), texCoord);
	outColor *= vec4(texCoord.x, 1.f, texCoord.y, 1.f);
}