struct Light
{
    float3 dir;
    float3 pos;
    float range;
    float3 att;
    float4 ambient;
    float4 diffuse;
};

cbuffer cbPerFrame
{
    Light light;
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

    float3 lightToPixelVec = light.pos - input.worldPos.xyz;
    float d = length(lightToPixelVec);
    if (d > light.range)
    {
        return float4(finalAmbient, matDiffuse.a);
    }

    // diffuse
    float3 finalDiffuse = float3(0.0f, 0.0f, 0.0f);
    lightToPixelVec /= d;
    float howMuchLight = dot(lightToPixelVec, input.normal);
    if (howMuchLight > 0.0f)
    {
        finalDiffuse += howMuchLight * matDiffuse * light.diffuse;
        finalDiffuse /= light.att[0] + (light.att[1] * d) + (light.att[2] * (d * d));
    }
    float3 finalColor = saturate(finalAmbient + finalDiffuse);
    return float4(finalColor, 0.0f);
}

float4 D2D_PS(VS_OUTPUT input) : SV_TARGET
{
    return ObjTexture.Sample(ObjSamplerState, input.TexCoord);
}