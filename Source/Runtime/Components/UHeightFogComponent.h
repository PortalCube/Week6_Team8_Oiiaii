#pragma once

#include "UPrimitiveComponent.h"
#include "Runtime/Core/FLinearColor.h"

struct FHeightFogSceneInfo
{
	float FogDensity = 0.3f;
	float FogHeightFalloff = 0.2f;
	float StartDistance = 10.f;
	float FogCutoffDistance = 0.f;
	float FogMaxOpacity = 1.0;
	FLinearColor FogInscatteringColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
};

class UHeightFogComponent : public UPrimitiveComponent
{
	DECLARE_UCLASS(UHeightFogComponent, UPrimitiveComponent)
	GENERATED_BODY()

public:
	void Serialize(FArchive& Archive) override;
	virtual EEngineShowFlags GetShowFlag() const override { return EEngineShowFlags::SF_Fog; }

	// ======== Getter & Setter ========
	float GetFogDensity() const { return Info.FogDensity; }
	void SetFogDensity(float InDensity) { Info.FogDensity = InDensity; }

	float GetFogHeightFalloff() const { return Info.FogHeightFalloff; }
	void SetFogHeightFalloff(float InFogHeightFalloff) { Info.FogHeightFalloff = InFogHeightFalloff; }

	float GetStartDistance() const { return Info.StartDistance; }
	void SetStartDistance(float InDistance) { Info.StartDistance = InDistance; }

	float GetFogCutoffDistance() const { return Info.FogCutoffDistance; }
	void SetFogCutoffDistance(float InDistance) { Info.FogCutoffDistance = InDistance; }

	float GetFogMaxOpacity() const { return Info.FogMaxOpacity; }
	void SetFogMaxOpacity(float InOpacity) { Info.FogMaxOpacity = InOpacity; }

	const FLinearColor& GetFogInscatteringColor() const { return Info.FogInscatteringColor; }
	void SetFogInscatteringColor(const FLinearColor& InColor) { Info.FogInscatteringColor = InColor; }

	float GetFogHeight() const { return GetGlobalTransform().GetLocation().Z; }

	float GetCameraHeightDensity(float CameraZ) const
	{
		return Info.FogDensity * expf(-1.f * Info.FogHeightFalloff * (CameraZ - GetFogHeight()));
	}

	// ======== Getter & Setter ========

private:
	FHeightFogSceneInfo Info;
};
