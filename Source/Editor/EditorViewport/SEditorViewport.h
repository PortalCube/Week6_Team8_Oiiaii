#pragma once

#include "FEditorViewportClient.h"
#include "Runtime/Engine/FViewport.h"
#include "Runtime/UI/SWindow.h"

// ViewportLayout의 Viewport 배열의 원소 (SWindow를 상속. Rect와 bVisible을 갖는다)
// 각 Viewport 하나를 나타낸다
struct SEditorViewport : public SWindow
{
private:
	FEditorViewportClient Client = {};	// SEditorViewport의 설정값, 카메라 등등을 담는다
	FViewport Viewport = {};			// SEditorViewport가 화면 어디에 그려져야 하는지 (위치, 크기), 어느 RTV에 그려져야 하는지를 담는다
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

	FViewport& GetViewport() { return Viewport; }
	FEditorViewportClient& GetClient() { return Client; }

	void SetSceneRect(const FRect& SceneRect);
	bool IsRenderable();
};
