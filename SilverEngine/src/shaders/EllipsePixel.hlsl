float4 main(float4 color : Color, float2 fragPos : FragPos) : SV_Target
{
    if (length(fragPos) > 0.5f)
        discard;
    return color;
}