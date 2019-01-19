// A constant buffer is basically a structure in an effect file 
// which holds variables we are able to update from our game code. 
cbuffer cbPerObject{
	float4x4 WVP;
	float4x4 World;
};

struct Light {
	float3 dir;
	float4 ambient;
	float4 diffuse;
};

cbuffer cbPerFrame {
	Light light;
};

Texture2D ObjTexture;
SamplerState ObjSamplerState;

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD;
	float3 normal : NORMAL;
};

VS_OUTPUT VS(float4 inPos : POSITION, float2 inTexCoord : TEXCOORD, float3 normal : NORMAL)
{
    VS_OUTPUT output;

    output.Pos = mul(inPos, WVP);
	output.TexCoord = inTexCoord;
	output.normal = mul(normal, World);	// ??? 法线转换到世界空间, 需要乘上顶点变换矩阵的逆转置矩阵.. 这里好像不是

	return output; 
} 

float4 PS(VS_OUTPUT input) : SV_TARGET
{
	input.normal = normalize(input.normal);

	float4 diffuse = ObjTexture.Sample(ObjSamplerState, input.TexCoord);

	float3 finalColor = diffuse * light.ambient + saturate(dot(light.dir, input.normal)) * light.diffuse * diffuse;

	return float4(finalColor, diffuse.a);
}

float4 D2D_PS(VS_OUTPUT input) : SV_TARGET
{
	return ObjTexture.Sample(ObjSamplerState, input.TexCoord);
}