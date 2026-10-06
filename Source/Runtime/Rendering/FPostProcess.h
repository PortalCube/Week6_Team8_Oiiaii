#pragma once

struct FFogSettings
{
	float FogDensity;
	float FogHeightFalloff;
	float StartDistance;
	float FogCutoffDistance;
	float FogMaxOpacity;
	FVector FogInscatteringColor;

	float GetCameraHeightDensity(float CameraZ, float FogComponentZ)
	{
		return FogDensity * expf(-1.f * FogHeightFalloff * (CameraZ - FogComponentZ));
	}
};

class FPostProcess
{
public:

private:

};

