Texture2D DepthTexture : register(t0);

struct PS_IN
{
	float4 Pos : SV_POSITION;
	float2 UV : TEXCOORD0;
};

// 각 뷰포트를 지정된 위치에 그리는 픽셀 셰이더
float4 MainPS(PS_IN Input) : SV_Target
{
	const float NearZ = 0.1f;
	const float FarZ = 100.f;
	const float VisMax = 10.f;

	// Input에 들어온 위치를 그대로 (SRV와 RTV의 크기가 동일함)
	int2 PixelCoord = int2(Input.Pos.xy);
	// Load()로 .r에 저장되어 있는 깊이 값만 빼오기
	float4 Loaded = DepthTexture.Load(int3(PixelCoord, 0));
	// NDC Z (Depth) 값 구하기
	float Depth = (NearZ * FarZ) / (FarZ - Loaded.r * (FarZ - NearZ));
	// log로 정규화 및 색상 반전
	float LinearDepth = 1 - saturate(log(Depth / NearZ) / log(VisMax / NearZ));
	// GrayScale로 출력
	return float4(LinearDepth, LinearDepth, LinearDepth, 1);
}
