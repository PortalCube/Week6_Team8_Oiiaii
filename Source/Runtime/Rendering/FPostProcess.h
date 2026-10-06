#pragma once

struct FFogSettings
{
	float FogDensity;
	float FogHeightFalloff;
	float StartDistance;
	float FogCutoffDistance;
	float FogMaxOpacity;
	FVector FogInscatteringColor;
	float FogHeight;	// 안개 기준 높이 (Fog Component Z)

	float GetCameraHeightDensity(float CameraZ, float FogComponentZ)
	{
		return FogDensity * expf(-1.f * FogHeightFalloff * (CameraZ - FogComponentZ));
	}
};

// ImGui에서 조절하는 전역 Fog 설정값
// TODO 컴포넌트로 옮기기
inline FFogSettings GFogSettings = {
	.FogDensity = 0.02f,
	.FogHeightFalloff = 0.06f,
	.StartDistance = 0.5f,
	.FogCutoffDistance = 0.f,
	.FogMaxOpacity = 0.5f,
	.FogInscatteringColor = FVector(0.447, 0.638, 1.0),
	.FogHeight = -5.f,
};

class FPostProcess
{
public:

private:

};

