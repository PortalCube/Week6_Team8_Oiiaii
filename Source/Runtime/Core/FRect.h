#pragma once

#include "Runtime/Math/FVector2.h"

struct FRect
{
	float Left, Top, Right, Bottom;

	bool operator==(const FRect&) const = default;

	float GetWidth() const { return Right - Left; }
	float GetHeight() const { return Bottom - Top; }
	FVector2 GetLeftTop() const { return FVector2{ Left, Top }; }
	FVector2 GetRightBottom() const { return FVector2{ Right, Bottom }; }
	FRect Round() const
	{
		FRect Rounded;
		Rounded.Left = roundf(Left);
		Rounded.Top = roundf(Top);
		Rounded.Right = roundf(Right);
		Rounded.Bottom = roundf(Bottom);
		return Rounded;
	}
};
