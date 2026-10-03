#include "SEditorViewport.h"

void SEditorViewport::SetSceneRect(const FRect& SceneRect)
{
	// 최소화 상태시 조기 리턴
	if (SceneRect.GetWidth() <= 0.0f || SceneRect.GetHeight() <= 0.0f)
	{
		return;
	}

	Viewport.SetLeftTop(SceneRect.GetLeftTop());
	Viewport.SetRightBottom(SceneRect.GetRightBottom());
	Client.GetViewportCamera().SetAspectRatio(SceneRect.GetWidth() / SceneRect.GetHeight());
}

bool SEditorViewport::IsRenderable()
{
	const FVector2 Size = Viewport.GetViewportSize();
	return bVisible && Size.X > 0.0f && Size.Y > 0.0f;
}
