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

	float3 V = WorldPos - CameraPos;
	float L = length(V);

	if (FogCutoffDistance != 0 && L > FogCutoffDistance || L <= StartDistance)
	{
		return SceneColor;
	}

	float DirZ = V.z / L;
	float NewDistance = L - StartDistance;

	float RayOriginDensity = CameraHeightDensity * exp(-FogHeightFalloff * StartDistance * DirZ);

	float K = FogHeightFalloff * NewDistance * DirZ;
	
	float ExtinctionCorrectionFactor = (abs(K) > 1e-4)
		? (1.f - exp(-K)) / K
		: 1.f - 0.5f * K + (1.f / 6.f) * K * K; // 테일러 근사
	
	float Tau = NewDistance * RayOriginDensity * ExtinctionCorrectionFactor;
	float T = exp(-1 * Tau);

	float FogFactor = max(T, 1.f - FogMaxOpacity);
	
	return float4(lerp(FogInscatteringColor, SceneColor.rgb, FogFactor), SceneColor.a);

}
