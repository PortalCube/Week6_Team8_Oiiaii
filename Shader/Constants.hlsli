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
	row_major float4x4 ViewProjectionInverse;
    float2 ViewportSize;
	float NearZ;
	float FarZ;
	float IsPerspective;
	float3 CameraPos;
}

cbuffer ObjectConstants : register(b2)
{
    float3 ColorOverride;
    float ColorOverrideAmount;
    float2 UVScale;
    float2 UVOffset;
    row_major float4x4 World;
    row_major float4x4 WorldInverseTranspose;
    float3 EmissiveColor;
	float DisableShading;
    float EmissiveIntensity;
}

cbuffer PostProcessConstants : register(b3)
{
	float VisMax;
	float VisMinOrtho;
	float VisMaxOrtho;
	float PostProcessPadding;
}

cbuffer LightConstants : register(b4)
{
    float3 LightDirection;
    float Intensity;
    float3 LightColor;
    float AmbientIntensity;
};

cbuffer HeightFogConstants : register(b7)
{
	float FogHeightFalloff;
	float CameraHeightDensity;
	float StartDistance;
	float FogCutoffDistance;
	float4 FogInscatteringColor;
}
