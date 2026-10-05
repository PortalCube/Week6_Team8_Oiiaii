#pragma once
#include "Runtime/UI/SWindow.h"
#include "Runtime/Core/FRect.h"
#include "Runtime/Core/PointerTypes.h"

struct FViewportRenderTarget;

struct FViewport
{
	FRect Rect = {}; // UV가 아닌 픽셀 좌표 Rect를 가지고 있음
	TSharedPtr<FViewportRenderTarget> RenderTarget;
	bool bResizeRenderTarget = false; // 리사이즈 되었을 때에만 RenderTarget의 Texture를 재생성. 매 프레임 Texture 재성성 막음

	// TSharedPtr<FViewportRenderTarget>의 소멸/대입에는 FViewportRenderTarget의 완전한 정의가 필요하다.
	// 헤더에는 전방 선언만 두고, 특수 멤버 함수는 FViewportRenderTarget.h를 include하는 FViewport.cpp에서 정의한다.
	FViewport();
	~FViewport();
	FViewport(const FViewport& Other);
	FViewport& operator=(const FViewport& Other);
	FViewport(FViewport&& Other) noexcept;
	FViewport& operator=(FViewport&& Other) noexcept;

	//====== Getter & Setter ======
	FVector2 GetLeftTop() const { return Rect.GetLeftTop(); }
	void SetLeftTop(const FVector2& InLeftTop) { Rect.Left = InLeftTop.X; Rect.Top = InLeftTop.Y; }

	FVector2 GetRightBottom() const { return Rect.GetRightBottom(); }
	void SetRightBottom(const FVector2& InRightBottom) { Rect.Right = InRightBottom.X; Rect.Bottom = InRightBottom.Y; }

	bool IsResizeRenderTarget() const { return bResizeRenderTarget; }
	void SetResizeRenderTarget(bool InbResizeRenderTarget) { bResizeRenderTarget = InbResizeRenderTarget; }
	//====== Getter & Setter ======


	// 뷰포트의 크기를 픽셀 단위로 반환
	FVector2 GetViewportSize() const { return GetRightBottom() - GetLeftTop(); }
};
