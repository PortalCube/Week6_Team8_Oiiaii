#pragma once
#include "Runtime/Math/FVector2.h"
#include "Runtime/Engine/FCamera.h"
#include "Editor/Grid/FGrid.h"
#include "Runtime/Engine/FViewport.h"
#include "Runtime/Engine/ShowFlags.h"
enum class ECameraMode
{
	PERSPECTIVE,
	ORTHOGRAPHIC,
	ORTHOGRAPHIC_TOP,
	ORTHOGRAPHIC_BOTTOM,
	ORTHOGRAPHIC_LEFT,
	ORTHOGRAPHIC_RIGHT,
	ORTHOGRAPHIC_FRONT,
	ORTHOGRAPHIC_BACK,
};

class FEditorViewportClient final
{
public:
	const ECameraMode GetCameraMode() const { return CameraMode; }
	// CameraMode에 따라 Camera의 설정을 변경
	void SetCameraMode(ECameraMode Mode);
	
	const EViewModeIndex const GetViewMode() const { return ViewMode; }
	void SetViewMode(EViewModeIndex InViewMode) { ViewMode = InViewMode; }

	FViewport& GetViewport() { return *Viewport; }
	const FViewport& GetViewport() const { return *Viewport; }
	void SetViewPort(FViewport* InViewport) { Viewport = InViewport; }

	FCamera& GetViewportCamera() { return ViewportCamera; }
	const FCamera& GetViewportCamera() const { return ViewportCamera; }
	void SetViewportCamera(FCamera InViewportCamera) { ViewportCamera = InViewportCamera; }

	void UpdateFocusedAndHovered(bool bFocused, bool bHovered);

	FGrid& GetGrid() { return Grid; }
	const FGrid& GetGrid() const { return Grid; }


	const uint64 GetShowFlags() const { return ShowFlags; }
	[[nodiscard]] bool HasShowFlag(EEngineShowFlags Flag) const
	{
		return (ShowFlags & static_cast<uint64>(Flag)) != 0;
	}
	void ToggleShowFlag(EEngineShowFlags Flag)
	{
		ShowFlags ^= static_cast<uint64>(Flag);
	}

	[[nodiscard]] bool IsFocused() const { return bFocused; }
	[[nodiscard]] bool IsHovered() const { return bHovered; }
	void Update();

	void ResizeViewport(const FRect& Rect)
	{
		ViewportCamera.SetAspectRatio(Rect.GetWidth() / Rect.GetHeight());

		// 픽셀. 창 크기가 바뀌어도 이 값은 그대로 쓸 수 있다.
		Viewport->SetLeftTop(FVector2{ Rect.Left , Rect.Top });
		Viewport->SetRightBottom(FVector2{ Rect.Right, Rect.Bottom});
	}

private:
	bool bFocused = false;
	bool bHovered = false;

	// 뷰포트
	FViewport* Viewport = nullptr;
	
	// 뷰포트 카메라
	FCamera ViewportCamera;

	ECameraMode CameraMode = ECameraMode::PERSPECTIVE;

	// 그리드
	FGrid Grid;
	
	// 쇼 플래그
	// 뷰포트 렌더 모드
	EViewModeIndex ViewMode = EViewModeIndex::VMI_Lit;
	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
	                   static_cast<uint64>(EEngineShowFlags::SF_BillboardText) |
	                   static_cast<uint64>(EEngineShowFlags::SF_Grid);
};
