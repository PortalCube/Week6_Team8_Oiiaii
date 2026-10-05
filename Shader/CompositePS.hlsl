Texture2D CompositeTexture : register(t0);
SamplerState CompositeSampler : register(s0);

struct PS_IN
{
	float4 Pos : SV_POSITION;
	float2 UV : TEXCOORD0;
};

// 각 뷰포트를 지정된 위치에 그리는 픽셀 셰이더
float4 MainPS(PS_IN Input) : SV_Target
{
	float4 Sampled = CompositeTexture.Sample(CompositeSampler, Input.UV);
	return Sampled;
}
