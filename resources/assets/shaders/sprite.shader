#name custom/Sprite
#type Sprite

// Vertex Shader
#begin SpriteVertex

SV_DEFINE_CAMERA(b0);

struct UserVertexOutput : VertexOutput
{
};

UserVertexOutput spriteVertex(VertexInput input)
{
    UserVertexOutput output;
	output.color = input.color;
	output.texCoord = input.texCoord;
	output.position = mul(camera.vpm, input.position);
    return output;
}

#end

// Pixel Shader
#begin SpriteSurface

SV_DEFINE_MATERIAL(b0) {
    matrix uwu;
    float nepee;
};

SV_TEXTURE(Texture, t1);

struct UserSurfaceInput : SurfaceInput
{
};

SurfaceOutput spriteSurface(UserSurfaceInput input)
{
    SurfaceOutput output;

    float4 texColor = _Albedo.Sample(sam, input.texCoord) * Texture.Sample(sam, input.texCoord);
    output.color = input.color * texColor;
	if (output.color.a < 0.05f) discard;

    output.color.r = nepee;

    return output;
}

#end