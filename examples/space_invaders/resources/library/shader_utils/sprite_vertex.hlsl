
#name SpriteVertex
#shadertype VertexShader

struct VertexInput {
	float4 position : Position;
	float2 texCoord : TexCoord;
	float4 color : Color;
};

struct VertexOutput {
	float4 color : FragColor;
	float2 texCoord : FragTexCoord;
	float4 position : SV_Position;
};

#userblock SpriteVertex

// Main
UserVertexOutput main(VertexInput input)
{
	return spriteVertex(input);
}
			