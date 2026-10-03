#pragma once
#include "Runtime/UI/SWindow.h"


struct FViewport
{
	// 전체 클라이언트 영역 기준 고정 UV: 좌상단 (0,0), 우하단 (1,1).
	// 픽셀 위치/크기는 사용할 때 클라이언트 크기를 곱해 계산한다.
	//FVector2 TopLeftUV = { 0.0f, 0.0f };
	//FVector2 LengthUV = { 1.0f, 1.0f };
	FRect Rect = {};

	bool bShow = false;

	FVector2 GetLeftTop() const { return Rect.GetLeftTop(); }
	void SetLeftTop(const FVector2& InLeftTop) { Rect.Left = InLeftTop.X; Rect.Top = InLeftTop.Y; }

	FVector2 GetRightBottom() const { return Rect.GetRightBottom(); }
	void SetRightBottom(const FVector2& InLengthUV) { Rect.Right = InLengthUV.X; Rect.Bottom = InLengthUV.Y; }
};
