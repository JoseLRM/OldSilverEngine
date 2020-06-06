Texture2D tex : register(t0);
SamplerState sam : register(s0);

float4 main(float2 texCoord : TexCoord) : SV_TARGET
{
    return tex.Sample(sam, texCoord);
}