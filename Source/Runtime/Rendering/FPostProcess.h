#pragma once

struct FFogSettings
{
	float FogDensity;
	float FogHeightFalloff;
	float StartDistance;
	float FogCutoffDistance;
	float FogMaxOpacity;
	FVector FogInscatteringColor;
	float FogHeight;

	float GetCameraHeightDensity(float CameraZ, float FogComponentZ)
	{
		return FogDensity * expf(-1.f * FogHeightFalloff * (CameraZ - FogComponentZ));
	}
};

// ImGui에서 조절하는 전역 Fog 설정값
// TODO 컴포넌트로 옮기기
inline FFogSettings GFogSettings = {
	.FogDensity = 0.3f,
	.FogHeightFalloff = 0.2f,
	.StartDistance = 10.f,
	.FogCutoffDistance = 0.f,
	.FogMaxOpacity = 1.f,
	.FogInscatteringColor = FVector(1.f, 1.f, 1.f),
	.FogHeight = -8.5f,
};

class FPostProcess
{
public:

private:

};

