#include "FEditorViewportClient.h"

void FEditorViewportClient::UpdateFocusedAndHovered(bool bFocused, bool bHovered)
{
	this->bFocused = bFocused;
	this->bHovered = bHovered;
	return;
}

// CameraMode에 따라 Camera의 설정을 변경
void FEditorViewportClient::SetCameraMode(ECameraMode Mode)
{
	// CameraMode에 따라 뷰포트 카메라의 EProjectionType도 변경
	if (Mode == ECameraMode::PERSPECTIVE)
	{
		ViewportCamera.SetProjectionType(EProjectionType::Perspective);
	}
	else
	{
		ViewportCamera.SetProjectionType(EProjectionType::Orthographic);
	}

	float distance = 7.0f;
	CameraMode = Mode;

	// ORTHOGRAPHIC 방향에 따라 카메라 기본 위치, 회전 값 세팅
	switch (Mode)
	{
	case ECameraMode::ORTHOGRAPHIC_TOP:
		ViewportCamera.SetPosition(FVector(0.0f, 0.0f, distance));
		ViewportCamera.SetRotation(-90.0f, 0.0f);
		break;
	case ECameraMode::ORTHOGRAPHIC_BOTTOM:
		ViewportCamera.SetPosition(FVector(0.0f, 0.0f, -distance));
		ViewportCamera.SetRotation(90.0f, 0.0f);
		break;
	case ECameraMode::ORTHOGRAPHIC_LEFT:
		ViewportCamera.SetPosition(FVector(0.0f, -distance, 0.0f));
		ViewportCamera.SetRotation(0.0f, 90.0f);
		break;

	case ECameraMode::ORTHOGRAPHIC_RIGHT:
		ViewportCamera.SetPosition(FVector(0.0f, distance, 0.0f));
		ViewportCamera.SetRotation(0.0f, -90.0f);
		break;

	case ECameraMode::ORTHOGRAPHIC_FRONT:
		ViewportCamera.SetPosition(FVector(distance, 0.0f, 0.0f));
		ViewportCamera.SetRotation(0.0f, 0.0f);
		break;

	case ECameraMode::ORTHOGRAPHIC_BACK:
		ViewportCamera.SetPosition(FVector(-distance, 0.0f, 0.0f));
		ViewportCamera.SetRotation(0.0f, 180.0f);
		break;
	}

}

// FEditorApplication::Render() 에서 필요한 FSceneView를 만들어 반환
FSceneView FEditorViewportClient::GetSceneView(const FLightConstants& InLightConstants)
{
	FSceneView SceneView{
		.Camera = ViewportCamera,
		.ViewProj = ViewportCamera.GetViewProjectionMatrix(),
		.LeftTopPixel = Viewport->GetLeftTop(),
		.RightBottomPixel = Viewport->GetRightBottom(),
		.ViewMode = ViewMode,
		.ShowFlags = ShowFlags,
		.LightConstants = InLightConstants
	};
	return SceneView;
}
