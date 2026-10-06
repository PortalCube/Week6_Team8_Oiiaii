#include "Constants.hlsli"

Texture2D DepthTexture : register(t0);

struct PS_IN
{
	float4 Pos : SV_POSITION;
	float2 UV : TEXCOORD0;
};

// 각 뷰포트를 지정된 위치에 그리는 픽셀 셰이더
float4 MainPS(PS_IN Input) : SV_Target
{
	// Input에 들어온 위치를 그대로 (SRV와 RTV의 크기가 동일함)
	int2 PixelCoord = int2(Input.Pos.xy);
	// Load()로 .r에 저장되어 있는 깊이 값만 빼오기
	float4 Loaded = DepthTexture.Load(int3(PixelCoord, 0));

	// 원근 투영
	if (IsPerspective > 0.5f)
	{
		// NDC Z (Depth) 값 구하기
		float Z = (NearZ * FarZ) / (FarZ - Loaded.r * (FarZ - NearZ));
		// log로 정규화 및 색상 반전 (Z가 비선형 이므로 정규화를 해야함)
		float LinearZ = 1 - saturate(log(Z / NearZ) / log(VisMax / NearZ));
		// GrayScale로 출력
		return float4(LinearZ, LinearZ, LinearZ, 1);
	}
	// 직교 투영
	else
	{
		// 직교 투영에서는 Z가 선형. 그대로 쓴다.
		float Z = 1 - Loaded.r;
		// 선형이어도 구분이 잘 되도록 정규화를 해준다
		float ContrastedZ = saturate((Z - VisMinOrtho) / (VisMaxOrtho - VisMinOrtho));
		return float4(ContrastedZ, ContrastedZ, ContrastedZ, 1);
	}
}
