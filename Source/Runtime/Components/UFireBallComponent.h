#pragma once

#include "UPrimitiveComponent.h"
#include "Runtime/Math/FVector4.h"

struct FPointLightConstants
{
	FVector PositionWS{ 0.0f, 0.0f, 0.0f };
	float Radius = 1.0f;

	FVector Color{ 1.0f, 1.0f, 1.0f };
	float Intensity = 3.0f;

	float FalloffExponent = 1.0f;
	float Padding[3]{};
};

static_assert(sizeof(FVector) == 12);
static_assert(sizeof(FPointLightConstants) == 48);

class UFireBallComponent : public UPrimitiveComponent
{
	DECLARE_UCLASS(UFireBallComponent, UPrimitiveComponent)
	GENERATED_BODY()

public:
	void Serialize(FArchive& Archive) const override;
	void Deserialize(const FArchive& Archive) override;

	FPointLightConstants GetPointLightData() const;

	float GetIntensity() const { return Intensity; }
	void SetIntensity(float InIntensity) { Intensity = InIntensity; }

	float GetRadius() const { return Radius; }
	void SetRadius(float InRadius) { Radius = InRadius; }

	float GetRadiusFalloff() const { return RadiusFalloff; }
	void SetRadiusFalloff(float InRadiusFalloff) { RadiusFalloff = InRadiusFalloff; }

	const FVector4& GetFireColor() const { return FireColor; }
	void SetFireColor(const FVector4& InColor) { FireColor = InColor; }

	const FVector& GetEmissiveColor() const { return EmissiveColor; }
	void SetEmissiveColor(const FVector& InColor) { EmissiveColor = InColor; }

	float GetEmissiveIntensity() const { return EmissiveIntensity; }
	void SetEmissiveIntensity(float InIntensity) { EmissiveIntensity = InIntensity; }

private:
	float Intensity = 1.0f;
	float Radius = 1.0f;
	float RadiusFalloff = 1.0f;
	FVector4 FireColor{ 1.0f, 1.0f, 1.0f, 1.0f };
	FVector EmissiveColor{ 1.0f, 1.0f, 1.0f };
	float EmissiveIntensity = 1.0f;
};
