struct INPUT
{
    float2 pos : Position;
    float4 color : Color;
};

struct OUTPUT
{
    float4 color : Color;
    
    float4 pos : SV_Position;
};

OUTPUT main(INPUT input)
{
    OUTPUT output;
    
    output.color = input.color;
    output.pos = float4(input.pos, 0.f, 1.f);
    
    return output;
}