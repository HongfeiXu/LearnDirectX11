struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

VS_OUTPUT VS(float4 inPos : POSITION, float4 inColor0 : COLOR_ZERO, float4 inColor1 : COLOR_ONE)
{
    VS_OUTPUT output;
    output.Pos = inPos;
    output.Color.r = inColor0.r * inColor1.r;
    output.Color.g = inColor0.g * inColor1.g;
    output.Color.b = inColor0.b * inColor1.b;
    output.Color.a = inColor0.a * inColor1.a;
	return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    return input.Color;
}