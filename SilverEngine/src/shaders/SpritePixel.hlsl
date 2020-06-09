Texture2D tex : register(t0);
SamplerState sam : register(s0);

float4 main(float2 texCoord : TexCoord, float4 color : Color) : SV_TARGET
{
    float4 textureColor = tex.Sample(sam, texCoord);
    return color * textureColor;
}