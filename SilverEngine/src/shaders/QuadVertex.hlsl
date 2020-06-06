struct INPUT
{
    float2 pos : Position;
    
    matrix tm : TM;
    float4 color : Color;
};

struct OUTPUT
{
    float4 color : Color;
    
    float4 pos : SV_Position;
};

OUTPUT main( INPUT input)
{
    OUTPUT output;
    
    output.color = input.color;
    output.pos = mul(float4(input.pos, 0.f, 1.f), input.tm);
    
	return output;
}