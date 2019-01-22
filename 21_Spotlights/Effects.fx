struct DirectionalLight
{
    float3 dir;
    float4 ambient;
    float4 diffuse;
    float4 specular;
};

struct SpotLight
{
    float3 pos;
    float innerCutOff;
    float3 dir;
    float outerCutOff;
    float3 att;
    float range;

    float4 ambient;
    float4 diffuse;
    float4 specular;
};

cbuffer cbPerFrame
{
    SpotLight light;
    float3 camWorldPos;
};

cbuffer cbPerObject
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInvTranspose; // World的逆转置矩阵,用于变换法线
};

Texture2D ObjTexture;
SamplerState ObjSamplerState;

//--------------------------------------------------------------------------------------
// 渲染物体, Blinn-Phong 光照模型
//--------------------------------------------------------------------------------------
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
    output.normal = mul(normal, (float3x3) WorldInvTranspose); // 法线转换到世界空间

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

    if(d > light.range)
        return float4(finalAmbient, matDiffuse.a);

    lightDir = lightDir / d;

    // diffuse
    float3 finalDiffuse = matDiffuse * light.diffuse * max(dot(lightDir, input.normal), 0);

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
    
    float attenuation = 1.0 / (light.att[0] + light.att[1] * d + light.att[2] * d * d);

    finalDiffuse *= attenuation;
    finalSpecular *= attenuation;

    float theta = dot(lightDir, -light.dir);
    float epsilon = light.innerCutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    finalDiffuse *= intensity;
    finalSpecular *= intensity;

    float3 finalColor = saturate(finalAmbient + finalDiffuse + finalSpecular);
    return float4(finalColor, matDiffuse.a);
}

//--------------------------------------------------------------------------------------
// 渲染文字
//--------------------------------------------------------------------------------------
float4 D2D_PS(VS_OUTPUT input) : SV_TARGET
{
    return ObjTexture.Sample(ObjSamplerState, input.TexCoord);
}

//--------------------------------------------------------------------------------------
// 渲染天空盒, 以物体的本地顶点坐标采样 SkyMap
//--------------------------------------------------------------------------------------
TextureCube SkyMap;

struct SKYMAP_VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};

SKYMAP_VS_OUTPUT SKYMAP_VS(float3 inPos : POSITION, float2 inTexCoord : TEXCOORD, float3 normal : NORMAL)
{
    SKYMAP_VS_OUTPUT output;
    //Set Pos to xyww instead of xyzw, so that z will always be 1 (furthest from camera)
    output.Pos = mul(float4(inPos, 1.0f), WVP).xyww;

    // Use our vertices position as a vector, describing the texel in our texture cube to color the pixel with.
    output.TexCoord = inPos;

    return output;
}

float3 SKYMAP_PS(SKYMAP_VS_OUTPUT input) : SV_TARGET
{
    return SkyMap.Sample(ObjSamplerState, input.TexCoord);
}

//--------------------------------------------------------------------------------------
// 渲染在物体反射天空盒
//--------------------------------------------------------------------------------------
struct REFLECT_VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
};

REFLECT_VS_OUTPUT REFLECT_VS(float3 inPos : POSITION, float2 inTexCoord : TEXCOORD, float3 normal : NORMAL)
{
    REFLECT_VS_OUTPUT output;

    output.Pos = mul(float4(inPos, 1.0f), WVP);

    output.worldPos = mul(inPos, (float3x3) World);
    output.normal = mul(normal, (float3x3) WorldInvTranspose); // 法线转换到世界空间

    return output;
}

float3 REFLECT_PS(REFLECT_VS_OUTPUT input) : SV_TARGET
{
    float3 viewDir = normalize(camWorldPos - input.worldPos.xyz);
    float3 reflectDir = reflect(-viewDir, input.normal);
    return SkyMap.Sample(ObjSamplerState, reflectDir);
}


//struct REFLECT_VS_OUTPUT
//{
//    float4 Pos : SV_POSITION;
//    float3 worldPos : POSITION;
//    float3 normal : NORMAL;
//};

//REFLECT_VS_OUTPUT REFLECT_VS(float3 inPos : POSITION, float2 inTexCoord : TEXCOORD, float3 normal : NORMAL)
//{
//    REFLECT_VS_OUTPUT output;

//    output.Pos = mul(float4(inPos, 1.0f), WVP);

//    output.worldPos = mul(inPos, (float3x3) World);
//    output.normal = mul(inPos, (float3x3) WorldInvTranspose); // 法线转换到世界空间

//    return output;
//}

//float3 REFLECT_PS(REFLECT_VS_OUTPUT input) : SV_TARGET
//{
//    float3 viewDir = normalize(camWorldPos - input.worldPos.xyz);
//    float3 reflectDir = reflect(-viewDir, input.normal);
//    return SkyMap.Sample(ObjSamplerState, reflectDir);
//}