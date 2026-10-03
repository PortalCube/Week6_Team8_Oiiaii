#pragma once
#include "Runtime/UI/SWindow.h"


struct FViewport
{
	// UV가 아닌 픽셀 좌표 Rect를 가지고 있음
	FRect Rect = {};
	bool bShow = false;

	bool operator==(const FViewport& Other) const = default;
	FViewport& operator=(const FViewport& Other) = default;

	//====== Getter & Setter ======
	FVector2 GetLeftTop() const { return Rect.GetLeftTop(); }
	void SetLeftTop(const FVector2& InLeftTop) { Rect.Left = InLeftTop.X; Rect.Top = InLeftTop.Y; }

	FVector2 GetRightBottom() const { return Rect.GetRightBottom(); }
	void SetRightBottom(const FVector2& InRightBottom) { Rect.Right = InRightBottom.X; Rect.Bottom = InRightBottom.Y; }
	//====== Getter & Setter ======


	// 뷰포트의 크기를 픽셀 단위로 반환
	FVector2 GetViewportSize() const { return GetRightBottom() - GetLeftTop(); }
};
