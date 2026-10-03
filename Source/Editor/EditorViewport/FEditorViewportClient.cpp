#include "FEditorViewportClient.h"

void FEditorViewportClient::UpdateFocusedAndHovered(bool bFocused, bool bHovered)
{
	this->bFocused = bFocused;
	this->bHovered = bHovered;
	return;
}
void FEditorViewportClient::SetCameraMode(ECameraMode Mode)
{
	if (Mode == ECameraMode::PERSPECTIVE)
	{
		ViewportCamera.SetProjectionType(EProjectionType::Perspective);
	}
	else
	{
		ViewportCamera.SetProjectionType(EProjectionType::Orthographic);
	}

	float distance = 5.0f;
	CameraMode = Mode;

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
