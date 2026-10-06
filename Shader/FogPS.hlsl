#include "Constants.hlsli"

Texture2D DepthTexture : register(t0);
Texture2D SceneColorTexture : register(t1);

struct PS_IN
{
	float4 Pos : SV_POSITION;
	float2 UV : TEXCOORD0;
};

float4 MainPS(PS_IN Input) : SV_Target
{
	int2 PixelCoord = int2(Input.Pos.xy);
	float4 Depth = DepthTexture.Load(int3(PixelCoord, 0));
	float4 SceneColor = SceneColorTexture.Load(int3(PixelCoord, 0));

	float NdcX = (Input.Pos.x / float(ViewportSize.x)) * 2.f - 1.f;
	float NdcY = 1.f - 2.f * (Input.Pos.y / float(ViewportSize.y));

	float4 World = mul(float4(NdcX, NdcY, Depth.r, 1), ViewProjectionInverse);
	float3 WorldPos = World.xyz / World.w;
	
	const float Extinc = .005f;
	const float4 FogColor = float4(1, 1, 1, 1);
	
	const float3 RayDirection = CameraPos - WorldPos;
	
	float T = clamp(exp(-1.f * (Extinc * length(RayDirection))), 0.f, 1.f);

	return lerp(FogColor, SceneColor, T);

}
