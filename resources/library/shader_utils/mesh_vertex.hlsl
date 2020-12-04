
#name MeshVertex
#shadertype VertexShader

struct VertexInput {
	float3 position : Position;
	float3 normal : Normal;
};

struct InstanceInput {
	matrix tm;
};

struct VertexOutput {
	float4 position : SV_Position;
};

SV_CONSTANT_BUFFER(InstanceInputBuffer, b0) {
	InstanceInput instanceInput;
};

// Free slots
#define FREE_CBUFFER_SLOT0 b1
#define FREE_CBUFFER_SLOT1 b2
#define FREE_CBUFFER_SLOT2 b3
#define FREE_CBUFFER_SLOT3 b4
#define FREE_CBUFFER_SLOT4 b5
#define FREE_CBUFFER_SLOT5 b6
#define FREE_CBUFFER_SLOT6 b7

#userblock MeshVertex

UserVertexOutput main(VertexInput vertexInput)
{
	return meshVertex(vertexInput, instanceInput);
}
			