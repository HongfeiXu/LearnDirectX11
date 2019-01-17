// A constant buffer is basically a structure in an effect file 
// which holds variables we are able to update from our game code. 
cbuffer cbPerObject{
	float4x4 WVP;
};

Texture2D ObjTexture;
SamplerState ObjSamplerState;

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

VS_OUTPUT VS(float4 inPos : POSITION, float2 inTexCoord : TEXCOORD)
{
    VS_OUTPUT output;
    output.Pos = mul(inPos, WVP);
	output.TexCoord = inTexCoord;
	return output; 
} 

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	return ObjTexture.Sample(ObjSamplerState, input.TexCoord);
}