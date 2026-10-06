#pragma once

#include "Runtime/Math/FMatrix.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Math/FVector4.h"

// Register = b0
struct FFrameConstants
{
	float Time;
	float DeltaTime;
	FVector2 Padding;
};
static_assert(sizeof(FFrameConstants) % 16 == 0);



// Register = b1
struct FViewConstants
{
	FMatrix View;
	FMatrix Projection;
	FMatrix ViewProjectionInverse;
	FVector2 ViewportSize;
	float NearZ;
	float FarZ;
	float IsPerspective;
	FVector CameraPos;
};
static_assert(sizeof(FViewConstants) % 16 == 0);



// Register = b2
struct FObjectConstants
{
	// FMatrix MVP;
	FVector4 Color{ 0.0f, 0.0f, 0.0f, 0.0f };
	FVector2 UVScale{ 1.0f, 1.0f };
	FVector2 UVOffset{ 0.0f, 0.0f };
	FMatrix World = FMatrix::GetIdentity();
	float DisableShading = 0.0f;
	FVector Padding;
	FMatrix WorldInverseTranspose = FMatrix::Identity;
	FVector EmissiveColor{ 0.0f, 0.0f, 0.0f };
	float EmissiveIntensity = 0.0f;
};
static_assert(sizeof(FObjectConstants) % 16 == 0);
static constexpr uint32 ConstantRangeAlignment = 256u;
static constexpr uint32 AlignConstantRange(uint32 Size)
{
	return (Size + ConstantRangeAlignment - 1u) & ~(ConstantRangeAlignment - 1u);
}
static constexpr uint32 ObjectConstantStride = AlignConstantRange(sizeof(FObjectConstants));
static_assert(ObjectConstantStride % 256u == 0);
static_assert(sizeof(FObjectConstants) <= ObjectConstantStride);
static constexpr uint32 MaxObjectDrawCount = 16384u;
static constexpr uint32 ObjectConstantUploadBufferSize = ObjectConstantStride * MaxObjectDrawCount;



// Register = b2 / ObjectConstants Override
// LINE_LIST 기반 에디터 그리드용 상수 버퍼
struct FGridLineConstants
{
	FMatrix MVP;
	FVector CameraPosition;
	float FadeStartDistance;
	float FadeEndDistance;
	FVector Padding;
};
static_assert(sizeof(FGridLineConstants) % 16 == 0);



// Register = b3
struct FPostProcessConstants
{
	float VisMax;		// 원근에서 SceneDepth의 최댓값
	float VisMinOrtho;	// 직교에서 SceneDepth의 최솟값
	float VisMaxOrtho;	// 직교에서 SceneDepth의 최댓값
	float Padding;
};
static_assert(sizeof(FPostProcessConstants) % 16 == 0);



// Register = b4
struct FPointLightCountConstants
{
	uint32 PointLightCount = 0;
	float Padding[3]{};
};
static_assert(sizeof(FPointLightCountConstants) == 16);



// 나중에 수정 필요
// Register = b4
struct FLightConstants
{
	// 기본 조명 파라미터
	FVector LightDirection{ -0.5f, -0.5f, -1.0f };
	float Intensity = 0.2f;

	FVector LightColor{ 1.0f, 1.0f, 1.0f };
	float AmbientIntensity = 0.05f;
};
static_assert(sizeof(FLightConstants) % 16 == 0);
