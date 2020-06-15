cbuffer TileMapBuffer
{
    matrix tm;
};

struct INPUT
{
    float2 position : Position;
    float2 texCoord : TexCoord;
    uint textureID : TextureID;
};

struct OUTPUT
{
    float2 texCoord : TexCoord;
    uint textureID : TextureID;
    
    float4 pos : SV_Position;
};

OUTPUT main(INPUT input)
{
    OUTPUT output;
    
    output.pos = mul(float4(input.position, 0.f, 1.f), tm);
    output.texCoord = input.texCoord;
    output.textureID = input.textureID;
    
	return output;
}