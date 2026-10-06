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
	float OutZ = 0.f;

	if (IsPerspective > 0.5f)
	{
		// NDC Z (Depth) 값 구하기
		float Z = (NearZ * FarZ) / (FarZ - Depth.r * (FarZ - NearZ));
		// log로 정규화 (Z가 비선형 이므로 정규화를 해야함)
		OutZ = saturate(log(Z / NearZ) / log(VisMax / NearZ));
	}
	// 직교 투영
	else
	{
		// 직교 투영에서는 Z가 선형. 그대로 쓴다.
		float Z = Depth.r;
		// 선형이어도 구분이 잘 되도록 정규화를 해준다
		OutZ = saturate((Z - VisMinOrtho) / (VisMaxOrtho - VisMinOrtho));
	}
	
	const float Extinc = .5f;
	const float4 FogColor = float4(1, 1, 1, 1);
	float T = exp(-1.f * (Extinc * OutZ) * (Extinc * OutZ));

	return lerp(FogColor, SceneColor, T);

}
