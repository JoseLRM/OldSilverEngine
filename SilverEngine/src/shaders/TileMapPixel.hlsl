Texture2D tex[10] : register(t0);

SamplerState sam0 : register(s0);
SamplerState sam1 : register(s1);
SamplerState sam2 : register(s2);
SamplerState sam3 : register(s3);
SamplerState sam4 : register(s4);
SamplerState sam5 : register(s5);
SamplerState sam6 : register(s6);
SamplerState sam7 : register(s7);
SamplerState sam8 : register(s8);
SamplerState sam9 : register(s9);

float4 SampleTexture(uint textureID, float2 texCoord)
{
    switch (textureID)
    {
        case 0:
            return tex[0].Sample(sam0, texCoord);
        case 1:
            return tex[1].Sample(sam1, texCoord);
        case 2:
            return tex[2].Sample(sam2, texCoord);
        case 3:
            return tex[3].Sample(sam3, texCoord);
        case 4:
            return tex[4].Sample(sam4, texCoord);
        case 5:
            return tex[5].Sample(sam5, texCoord);
        case 6:
            return tex[6].Sample(sam6, texCoord);
        case 7:
            return tex[7].Sample(sam7, texCoord);
        case 8:
            return tex[8].Sample(sam8, texCoord);
        case 9:
            return tex[9].Sample(sam9, texCoord);
        default:
            return float4(1.f, 1.f, 1.f, 1.f);
    }
}

float4 main(float2 texCoord : TexCoord, uint textureID : TextureID) : SV_TARGET
{    
    return SampleTexture(textureID, texCoord);
}