#include "core.hlsl"

// VERTEX SHADER
#ifdef SV_SHADER_TYPE_VERTEX

struct Input {
    float4 position : Position;
    float2 texCoord : TexCoord;
    float stroke : Stroke;
    float4 color : Color;
};

struct Output {
    float2 texCoord : PxTexCoord;
    float4 color : PxColor;
    float stroke : PxStroke;
    float4 position : SV_Position;
};

Output main(Input input)
{
    Output output;
    output.position = input.position;
    output.color = input.color;
    output.texCoord = input.texCoord;
    float2 halfSize = abs(input.texCoord);
    output.stroke = halfSize - (input.stroke * min(halfSize.x, halfSize.y));
    return output;
}

#endif

// PIXEL SHADER
#ifdef SV_SHADER_TYPE_PIXEL

struct Input {
    float2 texCoord : PxTexCoord;
    float4 color : PxColor;
    float stroke : PxStroke;
};

struct Output {
    float4 color : SV_Target;
};

Output main(Input input)
{
    float distance = length(input.texCoord);
    if (distance > 0.5f || distance < input.stroke) discard;
    
    Output output;
    output.color = input.color;
    return output;
}

#endif