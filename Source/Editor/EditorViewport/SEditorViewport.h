#pragma once

#include "FEditorViewportClient.h"
#include "Runtime/Engine/FViewport.h"
#include "Runtime/UI/SWindow.h"
#include "Runtime/Engine/FGameViewportClient.h"

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
	FGameViewportClient GameClient = {};
	bool bUsingGameClient = false;

public:
	SEditorViewport()
	{
		// 생성시 Client가 Viewport를 포인터로 가르키게
		Client.SetViewPort(&Viewport);

		GameClient.SetViewPort(&Viewport);
	}

	// 복사 대입 금지. Client가 포인터로 가르키고 있음
	SEditorViewport(const SEditorViewport&) = delete;
	SEditorViewport& operator=(const SEditorViewport&) = delete;

	FViewport& GetViewport() { return Viewport; }
	FEditorViewportClient& GetClient() { return Client; }

	FGameViewportClient& GetGameClient() { return GameClient; }
	bool IsPIE() const { return bUsingGameClient; }

	void AttachGameClient(FWorldContext* WorldContext, const FCamera& InitialCamera)
	{
		GameClient.SetWorldContext(WorldContext);
		GameClient.SetViewportCamera(InitialCamera);

		GameClient.SetCameraMode(ECameraMode::PERSPECTIVE);

		const FVector2 Size = Viewport.GetViewportSize();

		if (Size.X > 0.0f && Size.Y > 0.0f)
		{
			GameClient.GetViewportCamera().SetAspectRatio(Size.X / Size.Y);
		}

		Client.UpdateFocusedAndHovered(false, false);
		GameClient.UpdateFocusedAndHovered(false, false);

		 bUsingGameClient = true;
	}

	void DetachGameClient()
	{
		bUsingGameClient = false;

		GameClient.UpdateFocusedAndHovered(false, false);
		GameClient.SetWorldContext(nullptr);
	}

	FWorldContext* GetRenderWorldContext()
	{
		return bUsingGameClient ? GameClient.GetWorldContext() : Client.GetWorldContext();
	}

	FSceneView GetRenderSceneView(const FLightConstants& Light)
	{
		return bUsingGameClient ? GameClient.GetSceneView(Light) : Client.GetSceneView(Light);
	}

	void UpdateFocusedAndHovered(bool bFocused, bool bHovered)
	{
		Client.UpdateFocusedAndHovered(!bUsingGameClient && bFocused, !bUsingGameClient && bHovered);

		GameClient.UpdateFocusedAndHovered(bUsingGameClient && bFocused, bUsingGameClient && bHovered);
	}

	FCamera& GetActiveCamera()
	{
		return bUsingGameClient ? GameClient.GetViewportCamera() : Client.GetViewportCamera();
	}

	const FCamera& GetActiveCamera() const
	{
		return bUsingGameClient ? GameClient.GetViewportCamera() : Client.GetViewportCamera();
	}

	ECameraMode GetActiveCameraMode() const
	{
		return bUsingGameClient ? GameClient.GetCameraMode() : Client.GetCameraMode();
	}

	void SetActiveCameraMode(ECameraMode Mode)
	{
		if (bUsingGameClient)
		{
			GameClient.SetCameraMode(Mode);
		}
		else
		{
			Client.SetCameraMode(Mode);
		}
	}

	void SetSceneRect(const FRect& SceneRect);
	bool IsRenderable();
};
