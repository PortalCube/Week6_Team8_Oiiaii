#include "SEditorViewport.h"

void SEditorViewport::SetSceneRect(const FRect& SceneRect)
{
	// 최소화 상태시 조기 리턴
	if (SceneRect.GetWidth() <= 0.0f || SceneRect.GetHeight() <= 0.0f)
	{
		return;
	}
	FRect RoundedRect = SceneRect.Round();
	//<<<<<<<<<<<<<<
	// 반올림 후 크기가 0이 되면 종횡비 계산이 0으로 나누기가 되므로 조기 리턴
	if (RoundedRect.GetWidth() <= 0.0f || RoundedRect.GetHeight() <= 0.0f)
	{
		return;
	}

	// 크기가 바뀌었을 때만 렌더 타깃 재생성 플래그를 켠다
	const bool bSizeChanged =
	    Viewport.Rect.GetWidth() != RoundedRect.GetWidth() ||
	    Viewport.Rect.GetHeight() != RoundedRect.GetHeight();
	if (bSizeChanged)
	{
		Viewport.SetResizeRenderTarget(true);
	}

	// 위치와 크기는 항상 갱신한다.
	// (크기는 같고 위치만 바뀌는 경우: 툴바 숨김, 레이아웃 전환 등 → 합성 위치와 마우스 로컬 좌표가 어긋나지 않도록)
	Viewport.SetLeftTop(RoundedRect.GetLeftTop());
	Viewport.SetRightBottom(RoundedRect.GetRightBottom());
	//>>>>>>>>>>>>>>
	Client.GetViewportCamera().SetAspectRatio(RoundedRect.GetWidth() / RoundedRect.GetHeight());
}

bool SEditorViewport::IsRenderable()
{
	const FVector2 Size = Viewport.GetViewportSize();
	return bVisible && Size.X > 0.0f && Size.Y > 0.0f;
}
