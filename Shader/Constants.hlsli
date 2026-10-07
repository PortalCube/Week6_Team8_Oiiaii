cbuffer FrameConstants : register(b0)
{
    float Time;
    float DeltaTime;
    float2 FramePadding;
}

cbuffer ViewConstants : register(b1)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
    float2 ViewportSize;
	float NearZ;
	float FarZ;
	float IsPerspective;
	float3 ViewPadding;
}

cbuffer ObjectConstants : register(b2)
{
    row_major float4x4 MVP;
    float3 ColorOverride;
    float ColorOverrideAmount;
    float2 UVScale;
    float2 UVOffset;
    row_major float4x4 World;
    float DisableShading;
    float3 ObjectPadding;
    row_major float4x4 WorldInverseTranspose;
    float3 EmissiveColor;
    float EmissiveIntensity;
}

cbuffer PostProcessConstants : register(b3)
{
	float VisMax;
	float4 PostProcessPadding;
}

cbuffer LightConstants : register(b4)
{
    float3 LightDirection;
    float Intensity;
    float3 LightColor;
    float AmbientIntensity;
};
