struct INPUT
{
    float2 pos : Position;
    
    matrix tm : TM;
    float4 texCoord : TexCoord;
    float4 color : Color;
};

struct OUTPUT
{
    float2 texCoord : TexCoord;
    float4 color : Color;
    
    float4 pos : SV_Position;
};

OUTPUT main(INPUT input)
{
    OUTPUT output;
    
    output.color = input.color;
    float2 tc = input.pos + float2(0.5f, 0.5f);
    tc.y = 1.f - tc.y;
    output.texCoord = input.texCoord.xy + tc * input.texCoord.zw;
    output.pos = mul(float4(input.pos, 0.f, 1.f), input.tm);
    
	return output;
}