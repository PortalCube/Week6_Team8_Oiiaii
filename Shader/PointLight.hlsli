#ifndef POINT_LIGHT_HLSLI
#define POINT_LIGHT_HLSLI

struct FPointLightConstants
{
    float3 PositionWS;
    float Radius;

    float3 Color;
    float Intensity;

    float FalloffExponent;
    float3 Padding;
};


float3 EvaluatePointLight(FPointLightConstants Light, float3 PositionWS, float3 NormalWS, float3 BaseColor)
{
	float3 ToLight = Light.PositionWS - PositionWS;
	float Distance = length(ToLight);

	if (Light.Radius <= 0.0 || Distance >= Light.Radius)
	{
		return float3(0.0, 0.0, 0.0);
	}

	float3 LightDir = ToLight / max(Distance, 0.0001);

	float Attenuation = pow(saturate(1.0 - (Distance / Light.Radius)), max(Light.FalloffExponent, 0.0001));
	
	float3 NormalizedNormal = normalize(NormalWS);

	float Facing = saturate(dot(NormalizedNormal, LightDir));

	return BaseColor * Light.Color * Light.Intensity * Attenuation * Facing;
}

#endif // POINT_LIGHT_HLSLI
