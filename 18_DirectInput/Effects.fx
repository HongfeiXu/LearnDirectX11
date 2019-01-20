struct Light
{
    float3 dir;
    float3 pos;
    float range;
    float3 att;
    float4 ambient;
    float4 diffuse;
	// ===aha===
    float4 specular;
	// ===aha===
};

cbuffer cbPerFrame
{
    Light light;
	// ===aha===
    float3 camWorldPos;
	// ===aha===
};

cbuffer cbPerObject
{
    float4x4 WVP;
    float4x4 World;
};

Texture2D ObjTexture;
SamplerState ObjSamplerState;

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 worldPos : POSITION;
    float2 TexCoord : TEXCOORD;
    float3 normal : NORMAL;
};

VS_OUTPUT VS(float4 inPos : POSITION, float2 inTexCoord : TEXCOORD, float3 normal : NORMAL)
{
    VS_OUTPUT output;

    output.Pos = mul(inPos, WVP);
    output.TexCoord = inTexCoord;
    output.worldPos = mul(inPos, World);
    output.normal = mul(normal, World); // ??? 法线转换到世界空间, 需要乘上顶点变换矩阵的逆转置矩阵.. 这里好像不是

    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    input.normal = normalize(input.normal);

    // ambient
    float4 matDiffuse = ObjTexture.Sample(ObjSamplerState, input.TexCoord);
    float3 finalAmbient = matDiffuse * light.ambient;

    float3 lightDir = light.pos - input.worldPos.xyz;
    float d = length(lightDir);
    if (d > light.range)
    {
        return float4(finalAmbient, matDiffuse.a);
    }

    // diffuse
    float3 finalDiffuse = float3(0.0f, 0.0f, 0.0f);
    lightDir /= d;
    float howMuchLight = dot(lightDir, input.normal);
    if (howMuchLight > 0.0f)
    {
        finalDiffuse += howMuchLight * matDiffuse * light.diffuse;
        finalDiffuse /= light.att[0] + (light.att[1] * d) + (light.att[2] * (d * d));
    }

	// ===aha===
    float matGloss = 20.0;
    float4 matSpecular = float4(1.0, 1.0, 1.0, 1.0);

    // specular(Phong)
    //float3 reflectDir = normalize(reflect(-lightDir, input.normal));
    //float3 viewDir = normalize(camWorldPos - input.worldPos.xyz);
    //float3 finalSpecular = matSpecular * light.specular * pow(saturate(dot(reflectDir, viewDir)), matGloss);

    // specular(Blinn-Phong)
    float3 viewDir = normalize(camWorldPos - input.worldPos.xyz);
    float3 halfDir = normalize(viewDir + lightDir);
    float3 finalSpecular = matSpecular * light.specular * pow(saturate(dot(halfDir, input.normal)), matGloss);
	// ===aha===

    float3 finalColor = saturate(finalAmbient + finalDiffuse + finalSpecular);
    return float4(finalColor, matDiffuse.a);
}

float4 D2D_PS(VS_OUTPUT input) : SV_TARGET
{
    return ObjTexture.Sample(ObjSamplerState, input.TexCoord);
}