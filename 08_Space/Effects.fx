// A constant buffer is basically a structure in an effect file 
// which holds variables we are able to update from our game code. 
cbuffer cbPerObject{
	float4x4 WVP;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

VS_OUTPUT VS(float4 inPos : POSITION, float4 inColor : COLOR)
{
    VS_OUTPUT output;
    output.Pos = mul(inPos, WVP);
    output.Color= inColor;
	return output; 
} 

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    return input.Color;
}