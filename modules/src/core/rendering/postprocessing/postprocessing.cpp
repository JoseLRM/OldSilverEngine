#include "core.h"

#include "postprocessing_internal.h"

namespace sv {

	Shader* g_VS_DefPostProcessing = nullptr;
	Shader* g_PS_DefPostProcessing = nullptr;

	Result PostProcessing_internal::initialize()
	{
		svCheck(graphics_shader_compile_fastbin_from_string("PPShader_DefaultVertex", ShaderType_Vertex, &g_VS_DefPostProcessing, R"(
#include "core.hlsl"

struct Output {
	
	float2 texCoord : FragTexCoord;
	float4 position : SV_Position;

};

Output main(u32 id : SV_VertexID)
{
	Output output;

	output.texCoord = float2((id << 1) & 2, id & 2);
	output.position = float4(output.texCoord * float2(2, 2) + float2(-1, -1), 0, 1);

	return output;
}
		)"));

		svCheck(graphics_shader_compile_fastbin_from_string("PPShader_DefaultPixel", ShaderType_Pixel, &g_PS_DefPostProcessing, R"(
#include "core.hlsl"

struct Input {
	float2 texCoord : FragTexCoord;
};

struct Output {
	float4 color : SV_Target0;
};

SV_TEXTURE(Tex, t0);
SV_SAMPLER(Sam, s0);

Output main(Input input)
{
	Output output;
	output.color = Tex.Sample(Sam, input.texCoord);
	return output;
}
		)"));

		svCheck(pp_bloomInitialize());
		svCheck(pp_toneMappingInitialize());
		return Result_Success;
	}

	Result PostProcessing_internal::close()
	{
		svCheck(graphics_destroy(g_VS_DefPostProcessing));
		svCheck(graphics_destroy(g_PS_DefPostProcessing));

		svCheck(pp_toneMappingClose());
		svCheck(pp_bloomClose());
		return Result_Success;
	}

}