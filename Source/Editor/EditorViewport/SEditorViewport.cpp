#include "SEditorViewport.h"

void SEditorViewport::SetSceneRect(const FRect& SceneRect)
{
	// 최소화 상태시 조기 리턴
	if (SceneRect.GetWidth() <= 0.0f || SceneRect.GetHeight() <= 0.0f)
	{
		return;
	}
	FRect RoundedRect = SceneRect.Round();
	if (Viewport.Rect.GetWidth() != RoundedRect.GetWidth() || Viewport.Rect.GetHeight() != RoundedRect.GetHeight())
	{
		Viewport.SetLeftTop(RoundedRect.GetLeftTop());
		Viewport.SetRightBottom(RoundedRect.GetRightBottom());
		Viewport.bResizeRenderTarget = true;
	}
	Client.GetViewportCamera().SetAspectRatio(RoundedRect.GetWidth() / RoundedRect.GetHeight());
}

bool SEditorViewport::IsRenderable()
{
	const FVector2 Size = Viewport.GetViewportSize();
	return bVisible && Size.X > 0.0f && Size.Y > 0.0f;
}
