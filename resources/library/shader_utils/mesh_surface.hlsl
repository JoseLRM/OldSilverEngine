
#name MeshSurface
#shadertype PixelShader

struct SurfaceOutput {
	float3 diffuse : SV_Target0;
	float3 normal : SV_Target1;
};

#userblock MeshSurface

SurfaceOutput main(UserSurfaceInput input)
{
	return meshSurface(input);
}
			