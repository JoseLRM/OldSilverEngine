#name custom/Sprite
#type Sprite

// Vertex Shader
#begin SpriteVertex

SV_DEFINE_CAMERA(b0);

struct UserVertexOutput : VertexOutput
{
    float2 fragPos : FragPos;
};

UserVertexOutput spriteVertex(VertexInput input)
{
    UserVertexOutput output;
	output.color = input.color;
	output.texCoord = input.texCoord;
    output.fragPos = input.position.xy;
	output.position = mul(input.position, camera.vpm);
    return output;
}

#end

// Pixel Shader
#begin SpriteSurface

struct UserSurfaceInput : SurfaceInput
{
    float2 position : FragPos;
};

SurfaceOutput spriteSurface(UserSurfaceInput input)
{
    SurfaceOutput output;

    float4 texColor = _Albedo.Sample(sam, input.texCoord);
    output.color = input.color * texColor;
	if (output.color.a < 0.05f) discard;

    output.color.xyz *= lightingResult(input.position);

    return output;
}

#end