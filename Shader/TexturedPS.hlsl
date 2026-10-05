#include "Constants.hlsli"
#include "PointLight.hlsli"

Texture2D DiffuseTexture : register(t0);
SamplerState DiffuseSampler : register(s0);

StructuredBuffer<FPointLightConstants> PointLights : register(t3);

cbuffer PointLightCountConstants : register(b5)
{
    uint PointLightCount;
    float3 PointLightPadding;
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 Normal : NORMAL; // 법선
    float3 WorldPosition : TEXCOORD1;
};

float4 MainPS(PS_INPUT Input) : SV_Target
{
    float4 Sampled = DiffuseTexture.Sample(DiffuseSampler, Input.UV);
    
    // 하이라이트 색상 보간
    float3 Tint = lerp(float3(1.0f, 1.0f, 1.0f), ColorOverride, ColorOverrideAmount);
    float3 BaseColor = Sampled.rgb * Tint;

    if (DisableShading > 0.5f)
    {
        return float4(BaseColor, Sampled.a);
    }
    
    // 조명 계산 및 양면 음영 보정
    float3 N = normalize(Input.Normal);
    float NdotL = max(0.0f, dot(N, -normalize(LightDirection)));
    float3 Diffuse = LightColor * (Intensity * NdotL);
    float3 Ambient = LightColor * AmbientIntensity;
    float3 DirectionalLight = Ambient + Diffuse;

    float3 FinalColor = BaseColor * DirectionalLight;

    for (uint i = 0; i < PointLightCount; ++i)
    {
        FinalColor += EvaluatePointLight(PointLights[i], Input.WorldPosition, N, BaseColor);
    }

    return float4(FinalColor, Sampled.a);
}
