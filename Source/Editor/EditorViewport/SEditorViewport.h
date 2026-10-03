#pragma once

#include "FEditorViewportClient.h"
#include "Runtime/Engine/FViewport.h"
#include "Runtime/UI/SWindow.h"

constexpr uint32 MAX_VIEWPORT_COUNT = 4;

// ViewportLayout의 leaf (SWindow)
struct SEditorViewport : public SWindow
{
private:
	FEditorViewportClient Client = {};
	FViewport Viewport = {};
	float HeaderHeight = 0.f;
	// SWindow 상속
	// FRect Rect
	// bool bVisible

public:
	SEditorViewport()
	{
		// 생성시 Client가 Viewport를 포인터로 가르키게
		Client.SetViewPort(&Viewport);
	}

	// 복사 대입 금지. Client가 포인터로 가르키고 있음
	SEditorViewport(const SEditorViewport&) = delete;
	SEditorViewport& operator=(const SEditorViewport&) = delete;

	bool operator==(const SEditorViewport&) const = default;

	FViewport& GetViewport() { return Viewport; }
	FEditorViewportClient& GetClient() { return Client; }

	void SetSceneRect(const FRect& SceneRect)
	{
		if (SceneRect.GetWidth() <= 0.0f || SceneRect.GetHeight() <= 0.0f)
		{
			return;
		}

		Viewport.SetLeftTop(SceneRect.GetLeftTop());
		Viewport.SetRightBottom(SceneRect.GetRightBottom());
		Client.GetViewportCamera().SetAspectRatio(SceneRect.GetWidth() / SceneRect.GetHeight());
	}
};
