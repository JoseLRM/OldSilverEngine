struct OUTPUT
{
    float2 texCoord : TexCoord;
    float4 pos : SV_Position;
};

OUTPUT main( float2 pos : Position )
{
    OUTPUT output;
    
    float2 position = (pos + 1.f) / 2.f;
    position.y = 1.f - position.y;
    output.texCoord = position;
    output.pos = float4(pos, 0.f, 1.f);
    
    return output;
}