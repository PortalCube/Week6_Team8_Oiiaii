#include "UAnimatedBillboardComp.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Serialization/FArchive.h"
#include "Runtime/CoreUObject/UClass.h"
#include <algorithm>

IMPLEMENT_UCLASS(UAnimatedBillboardComp, UBillboardComponent)
UCLASS_META(UAnimatedBillboardComp, DisplayName, "Animated Billboard Component")

void UAnimatedBillboardComp::Initialize()
{
	Super::Initialize();
	bTickEnabled = true;
}

void UAnimatedBillboardComp::SetSpriteSheet(int InGridX, int InGridY,
	float InFrameRate,
	int InTotalFrames)
{
	GridX = (InGridX > 0) ? InGridX : 1;
	GridY = (InGridY > 0) ? InGridY : 1;
	FrameRate = (InFrameRate > 0.0f) ? InFrameRate : 1.0f;

	if (InTotalFrames > 0)
	{
		TotalFrames = InTotalFrames;
	}
	else
	{
		TotalFrames = GridX * GridY;
	}

	CurrentFrame = 0;
	ElapsedTime = 0.0f;
	RefreshUV();
}

void UAnimatedBillboardComp::Stop()
{
	bPlaying = false;
	CurrentFrame = 0;
	ElapsedTime = 0.0f;
	RefreshUV();
}

void UAnimatedBillboardComp::SetCurrentFrame(int InFrame)
{
	if (TotalFrames > 0)
	{
		CurrentFrame = std::clamp(InFrame, 0, TotalFrames - 1);
		RefreshUV();
	}
}

void UAnimatedBillboardComp::Serialize(FArchive& Archive)
{
	Super::Serialize(Archive);

	Archive.Field("GridX", GridX);
	Archive.Field("GridY", GridY);
	Archive.Field("TotalFrames", TotalFrames);
	// Archive.Field("CurrentFrame", CurrentFrame);
	Archive.Field("FrameRate", FrameRate);
	// Archive.Field("ElapsedTime", ElapsedTime);
	// Archive.Field("Playing", bPlaying);
	Archive.Field("Loop", bLoop);
	Archive.Field("CurrentUVScale", CurrentUVScale);
	Archive.Field("CurrentUVOffset", CurrentUVOffset);
}

void UAnimatedBillboardComp::TickComponent(float DeltaTime)
{
	if (!bPlaying || TotalFrames <= 1 || FrameRate <= 0.0f)
	{
		return;
	}

	ElapsedTime += DeltaTime;
	const float FrameDuration = 1.0f / FrameRate;

	while (ElapsedTime >= FrameDuration)
	{
		ElapsedTime -= FrameDuration;
		CurrentFrame++;

		if (CurrentFrame >= TotalFrames)
		{
			if (bLoop)
			{
				CurrentFrame = 0;
			}
			else
			{
				CurrentFrame = TotalFrames - 1;
				bPlaying = false;
				break;
			}
		}
	}

	RefreshUV();
}

void UAnimatedBillboardComp::RefreshUV()
{
	if (GridX <= 0 || GridY <= 0)
	{
		SetUVScale(FVector2{ 1.0f, 1.0f });
		SetUVOffset(FVector2{ 0.0f, 0.0f });
		return;
	}

	const FVector2 UVScale{
		1.0f / static_cast<float>(GridX),
		1.0f / static_cast<float>(GridY)
	};

	const int Col = CurrentFrame % GridX;
	const int Row = CurrentFrame / GridX;

	SetUVScale(UVScale);
	SetUVOffset(FVector2{
	    static_cast<float>(Col) * UVScale.X,
	    static_cast<float>(Row) * UVScale.Y });
}
