#include "FImguiEditorViewportWindow.h"

#include "Runtime/Actors/AActor.h"
#include "Runtime/Components/UPrimitiveComponent.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Core/FRect.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/FStatsManager.h"
#include "Runtime/Engine/FRayCastingManager.h"
#include "Runtime/Engine/FSceneBVH.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Math/FVector.h"


#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"

void FImguiEditorViewportWindow::Process(FEditor& Editor, float DeltaTime)
{
	// 화면이 버튼을 눌러 최대일때 처리
	ApplyPendingViewportMaximize(Editor);

	// 종료와 Hover 초기화는 뷰포트의 포커스/표시 여부와 관계없이 처리한다.
	FGizmo& Gizmo = Editor.GetGizmo();
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
		Gizmo.EndInteraction();
	if (!Gizmo.IsInteracting())
		Gizmo.HoveredHandle = EGizmoHandle::None;

	FEditorViewportLayout& Layout = Editor.GetViewportLayout();
	SEditorViewport* ActiveViewport = Editor.GetActiveViewport();

	const ImGuiViewport* MainViewport = ImGui::GetMainViewport();
	const FVector2 ClientSize{ MainViewport->Size.x, MainViewport->Size.y };

	BeginWindow();

	// 부모 창의 콘텐츠 영역
	const ImVec2 ContentPos = ImGui::GetCursorScreenPos();
	const ImVec2 ContentSize = ImGui::GetContentRegionAvail();
	const ImVec2 Origin = MainViewport->Pos;

	if (ClientSize.X <= 0.0f || ClientSize.Y <= 0.0f || ContentSize.x <= 0.0f || ContentSize.y <= 0.0f)
	{
		EndWindow();
		return;
	}
	// 부모 콘텐츠 영역을 기존 Leaf 좌표계로 변환
	const FRect Rect{
		ContentPos.x - Origin.x,
		ContentPos.y - Origin.y,
		ContentPos.x - Origin.x + ContentSize.x,
		ContentPos.y - Origin.y + ContentSize.y
	};
	Layout.Resize(Rect);

	// 스플리터 입력 및 Leaf 영역 갱신
	if (Layout.SplitterV.bVisible)
		ShowViewportVerticalSplitter(Layout.SplitterV);
	if (Layout.SplitterH1.bVisible)
		ShowViewportHorizontalSplitter(Layout.SplitterH1);
	if (Layout.SplitterH2.bVisible)
	{
		Layout.SplitterH2.Ratio = Layout.SplitterH1.Ratio;
		ShowViewportHorizontalSplitter(Layout.SplitterH2);
		Layout.SplitterH1.Ratio = Layout.SplitterH2.Ratio;
	}

	// 연결된 스플리터 비율을 이번 프레임의 모든 Leaf에 반영한다.
	Layout.Resize(Rect);

	// 스플리터 비율 저장
	const FVector SplitterRatio = Layout.GetSplitterRatio();
	Editor.State.SetSplitter(SplitterRatio.X, SplitterRatio.Y, SplitterRatio.Z);

	// 자식 창 안에서 수집하고, 루프 뒤에서 한 번만 처리할 입력
	FViewportInput ActiveInput{};
	bool bHasActiveInput = false;

	for (SEditorViewport& EditorViewport : Layout.Viewports)
	{
		if (!EditorViewport.bVisible)
			continue;

		// 내부 경계에만 스플리터 공간을 확보해 3D 입력 영역과 겹치지 않게 한다.
		FRect ChildRect = EditorViewport.Rect;
		if (ChildRect.Left > Rect.Left)
			ChildRect.Left += 3.0f;
		if (ChildRect.Right < Rect.Right)
			ChildRect.Right -= 3.0f;
		if (ChildRect.Top > Rect.Top)
			ChildRect.Top += 3.0f;
		if (ChildRect.Bottom < Rect.Bottom)
			ChildRect.Bottom -= 3.0f;

		const float Width = ChildRect.GetWidth();
		const float Height = ChildRect.GetHeight();

		if (Width <= 0.0f || Height <= 0.0f)
			continue;

		// 스플리터 여백을 제외한 자식 창 위치를 ImGui 화면 좌표로 변환
		ImGui::SetCursorScreenPos(ImVec2(Origin.x + ChildRect.Left, Origin.y + ChildRect.Top));

		// 자식 창과 내부 UI의 ID를 뷰포트별로 분리
		ImGui::PushID(&EditorViewport);

		const bool bVisible = ImGui::BeginChild(
		    "ViewportChild",
		    ImVec2(Width, Height),
		    ImGuiChildFlags_None,
		    ImGuiWindowFlags_NoBackground |
		        ImGuiWindowFlags_NoScrollbar |
		        ImGuiWindowFlags_NoScrollWithMouse);

		if (bVisible)
		{
			// 상단바 생성
			DrawViewportHeader(EditorViewport, Editor);

			// 상단바 아래의 실제 3D 영역을 별도 함수로 계산
			FRect SceneRect{};
			if (GetViewportSceneRect(Origin, SceneRect))
			{
				// 상단바를 제외한 영역으로 렌더링·종횡비 설정
				EditorViewport.SetSceneRect(SceneRect);
				const FVector2 ViewportSizePixels = EditorViewport.GetViewport().GetViewportSize();
				const FVector2 LeftTopPixel = EditorViewport.GetViewport().GetLeftTop();
				FViewportInput Input = GatherInput(ViewportSizePixels, LeftTopPixel);

				// 3D 입력 아이템을 누른 경우에만 활성 뷰포트를 변경한다.
				if (Input.bPickRequested || ImGui::IsItemClicked(ImGuiMouseButton_Right))
				{
					Layout.SetActiveViewport(&EditorViewport);
					ActiveViewport = Editor.GetActiveViewport();
					ImGui::SetWindowFocus();
					Input.bFocused = true;
				}

				// 클릭 전에도 마우스가 올라간 뷰포트에서 매 프레임 검사한다.
				if (Editor.ObjectSelected() && Input.bHovered && !Gizmo.IsInteracting())
				{
					UpdateGizmoHover(Editor, EditorViewport, Input.LocalMouse, Input.SizePixels);
				}

				// 조작은 활성 뷰포트에서 한 번만 처리한다.
				if (&EditorViewport == ActiveViewport)
				{
					ActiveInput = Input;
					bHasActiveInput = true;

					// 스탯 오버레이는 활성 뷰포트에만 그린다. 분할 뷰에서
					// leaf마다 그리면 같은 패널이 화면 수만큼 중복된다.
					StatsWindow.Process(Editor, DeltaTime);
				}
			}
		}

		// BeginChild 반환값과 관계없이 반드시 호출
		ImGui::EndChild();
		ImGui::PopID();
	}

	// 활성 뷰포트의 입력을 한 번만 처리
	if (ActiveViewport && bHasActiveInput)
	{
		ActiveViewport->GetClient().UpdateFocusedAndHovered(ActiveInput.bFocused, ActiveInput.bHovered);
		UpdateSelection(Editor, *ActiveViewport, ActiveInput);
		UpdateGizmo(Editor, ActiveInput);
		UpdateCamera(Editor, *ActiveViewport, ActiveInput, DeltaTime);
	}
	// 현재 ImGui 창은 다시 부모 창
	ClampWindowToWorkArea();
	EndWindow();
}
void FImguiEditorViewportWindow::Toggle(FImguiStatsWindow::EStatsWindow Window)
{
	StatsWindow.Toggle(Window);
}

void FImguiEditorViewportWindow::SetClose()
{
	StatsWindow.SetClose();
}

void FImguiEditorViewportWindow::BeginWindow() const
{
	constexpr ImGuiWindowFlags WindowFlags =
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
	    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground |
	    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(30.0f, 30.0f));

	ImGui::Begin("Viewport", nullptr, WindowFlags);

	// 3D 는 이 창 아래에 그려지므로 창 자체는 항상 가장 뒤에 둔다.
	ImGui::BringWindowToDisplayBack(ImGui::GetCurrentWindow());

	ImGui::PopStyleVar(3);
}

void FImguiEditorViewportWindow::EndWindow() const
{
	ImGui::End();
}

FImguiEditorViewportWindow::FViewportInput FImguiEditorViewportWindow::GatherInput(const FVector2& ViewportSizePixels, const FVector2& ViewportLeftTopPixels) const
{
	// 상단바 아래 3D 영역만 등록한다. 드래그 중에는 영역 밖에서도 활성 상태를 유지한다.
	ImGui::InvisibleButton("ViewportInput", ImVec2(ViewportSizePixels.X, ViewportSizePixels.Y), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

	FViewportInput Input;
	Input.SizePixels = ViewportSizePixels;
	Input.LocalMouse = FInputManager::Get().GetMousePosition() - ViewportLeftTopPixels;
	Input.bHovered = ImGui::IsItemHovered();
	Input.bFocused = ImGui::IsWindowFocused();
	Input.bPickRequested = ImGui::IsItemClicked(ImGuiMouseButton_Left);
	Input.bLeftDown = ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left);
	Input.bLeftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);

	return Input;
}

void FImguiEditorViewportWindow::ClampWindowToWorkArea() const
{
	const float WorkTop = ImGui::GetMainViewport()->WorkPos.y;
	const ImVec2 WindowPos = ImGui::GetWindowPos();

	if (WindowPos.y < WorkTop)
	{
		ImGui::SetWindowPos(ImVec2(WindowPos.x, WorkTop));
	}
}

void FImguiEditorViewportWindow::UpdateSelection(FEditor& Editor, SEditorViewport& EditorViewport, const FViewportInput& Input)
{
	if (Input.bPickRequested)
	{
		HandlePicking(Editor, EditorViewport, Input.LocalMouse, Input.SizePixels);
	}
}

void FImguiEditorViewportWindow::UpdateGizmo(FEditor& Editor, const FViewportInput& Input)
{
	FGizmo& Gizmo = Editor.GetGizmo();

	// Hover와 종료는 Process에서 처리하고, 여기서는 진행 중인 드래그만 갱신한다.
	if (Editor.ObjectSelected() && Gizmo.IsInteracting() && Input.bLeftDown)
	{
		Gizmo.UpdateInteraction(Editor, Input.LocalMouse);
	}
}

void FImguiEditorViewportWindow::UpdateCamera(FEditor& Editor, SEditorViewport& EditorViewport,
    const FViewportInput& Input, float DeltaTime)
{
	if (!Input.bFocused)
	{
		CameraController.ResetVelocity();
		return;
	}

	CameraController.CameraRotateSpeed = Editor.State.GetCameraSensitivity();
	CameraController.CameraMoveSpeed = Editor.State.GetCameraSpeed();

	FCamera& Camera = EditorViewport.GetClient().GetViewportCamera();

	// ORTHOGRAPHIC 화면모드와의 분기
	const bool bOrthographic = Camera.GetProjection().GetProjectionType() == EProjectionType::Orthographic;
	if (bOrthographic)
	{
		CameraController.UpdateMouseInput_ORTHOGRAPHIC(Camera, Input.SizePixels.Y, Input.bHovered);
	}
	else
	{
		CameraController.UpdateMouseInput(Camera);
	}

	// 우클릭 중에는 WASD 가 카메라 비행에 쓰이므로 단축키와 겹치지 않게 나눈다.
	// 우클릭 중에만 WASD가 동작하도록. 언리얼 에디터 스타일
	if (FInputManager::Get().IsMousePressed(EMouseButton::Right))
	{
		if (!bOrthographic) // ORTHOGRAPHIC이 아닌 경우에만 WASD가 움직임
		{
			CameraController.UpdateKeyInput(Camera, DeltaTime);
		}
		return;
	}
	else
	{
		CameraController.ResetVelocity();
	}

	// 기즈모를 드래그하는 중에는 모드가 바뀌면 안 된다.
	if (!Editor.GetGizmo().IsInteracting())
	{
		UpdateShortcuts(Editor);
	}
}

void FImguiEditorViewportWindow::UpdateShortcuts(FEditor& Editor) const
{
	FInputManager& Input = FInputManager::Get();
	FGizmo& Gizmo = Editor.GetGizmo();

	// 백틱(`) : 월드/로컬 공간 전환.
	// Translate/Rotate 에서만 의미가 있어 None/Scale 은 제외한다.
	if (Input.IsKeyDown(VK_OEM_3))
	{
		if (Gizmo.Mode != EGizmoMode::None && Gizmo.Mode != EGizmoMode::Scale)
		{
			Gizmo.SetGizmoSpace(
			    static_cast<EGizmoSpace>((static_cast<uint8>(Gizmo.GetSpace()) + 1) % 2));
		}
	}

	if (Input.IsKeyDown('Q'))
	{
		Gizmo.Mode = EGizmoMode::None;
	}
	else if (Input.IsKeyDown('W'))
	{
		Gizmo.Mode = EGizmoMode::Translate;
	}
	else if (Input.IsKeyDown('E'))
	{
		Gizmo.Mode = EGizmoMode::Rotate;
	}
	else if (Input.IsKeyDown('R'))
	{
		Gizmo.Mode = EGizmoMode::Scale;
	}
	else if (Input.IsKeyDown(VK_SPACE))
	{
		Gizmo.Mode = static_cast<EGizmoMode>((static_cast<uint8>(Gizmo.Mode) + 1) % 4);
	}
}

void FImguiEditorViewportWindow::HandlePicking(FEditor& Editor,
    SEditorViewport& EditorViewport,
    const FVector2& LocalMousePixels,
    const FVector2& ViewportSizePixels)
{
	// 기즈모 핸들 위를 눌렀으면 피킹 대신 조작을 시작한다.
	if (Editor.GetSelectedActor() != nullptr)
	{
		// Process에서 현재 뷰포트의 Hover 판정을 먼저 갱신한 상태다.
		FGizmo& Gizmo = Editor.GetGizmo();

		if (Gizmo.HoveredHandle != EGizmoHandle::None)
		{
			Gizmo.BeginInteraction(
			    Editor.SelectedTransform,
			    Gizmo.HoveredHandle,
			    LocalMousePixels,
			    EditorViewport.GetClient().GetViewportCamera(),
			    ViewportSizePixels);

			return;
		}
	}

	UPrimitiveComponent* HitComponent = nullptr;
	FVector ImpactPoint;
	bool bHit = false;

	ULevel* PickScene = Editor.GetCurrentLevel();

	// 1) 마우스 화면 좌표 획득
	// 2) 화면 좌표 -> 월드 좌표로의 픽 레이(Pick Ray) 계산
	const FRay PickRay = FRayCastingManager::CreateRayFromScreenPosition(
	    EditorViewport.GetClient().GetViewportCamera(), LocalMousePixels, ViewportSizePixels);

	// 벤치마크용 광선 저장 (측정 구간 밖)
	FRayCastingManager::LastPickRay = PickRay;
	FRayCastingManager::bHasLastPickRay = true;

	// 3) 퍼포먼스 측정용 카운터 시작
	FScopeCycleCounter PickCounter;

	// 4) 전체 Picking 횟수 누적
	++Editor.PickingAttempts;

	// 5) 모든 오브젝트(프리미티브)에 대해 충돌 판정
	if (Editor.bUseBVHPicking && PickScene)
	{
		bHit = PickScene->GetSceneBVH().QueryRay(PickRay, HitComponent, ImpactPoint);
	}
	else
	{
		const TArray<UPrimitiveComponent*>& Components = Editor.GetPrimitiveComponents();
		bHit = FRayCastingManager::RayIntersectsMeshes(
		    PickRay, EditorViewport.GetClient().GetViewportCamera(), Components, HitComponent, ImpactPoint);
	}

	// 6) 퍼포먼스 측정 종료 및 시간 누적
	Editor.LastPickingMs = PickCounter.Finish();
	Editor.AccumulatedPickingMs += Editor.LastPickingMs;

	// 필요 시 'isHit' 결과를 활용해 추가 로직 처리
	// 피킹은 액터 단위로 선택한다. 소유 액터가 없으면 선택할 수 없다.
	if (!bHit || !HitComponent || !HitComponent->GetActorOwner())
	{
		Editor.UnSelectActor();
		return;
	}

	AActor* OwnerActor = HitComponent->GetActorOwner();
	Editor.SelectActor(OwnerActor);

	const char* ActorClass =
	    OwnerActor->GetClass() ? OwnerActor->GetClass()->GetDisplayName().c_str() : "Unknown";
	const char* CompClass =
	    HitComponent->GetClass() ? HitComponent->GetClass()->GetDisplayName().c_str() : "Unknown";

	UE_LOG("[Picking] Actor: %s (UUID: %u), Component: %s (UUID: %u)", ActorClass,
	    OwnerActor->GetUUID(), CompClass, HitComponent->GetUUID());
}

void FImguiEditorViewportWindow::UpdateGizmoHover(FEditor& Editor,
    SEditorViewport& EditorViewport,
    const FVector2& LocalMousePixels,
    const FVector2& ViewportSizePixels)
{
	FRay Ray = FRayCastingManager::CreateRayFromScreenPosition(
	    EditorViewport.GetClient().GetViewportCamera(), LocalMousePixels, ViewportSizePixels);

	FGizmo& Gizmo = Editor.GetGizmo();
	Gizmo.HoveredHandle = Gizmo.HitTest(Editor.SelectedTransform, Ray, EditorViewport.GetClient().GetViewportCamera());
}

void FImguiEditorViewportWindow::ShowViewportVerticalSplitter(SSplitter& Splitter)
{ // SplitterV용
	const FRect& R = Splitter.Rect;
	const ImVec2 Origin = ImGui::GetMainViewport()->Pos;

	float Top = R.GetHeight() * Splitter.Ratio;
	float Bottom = R.GetHeight() - Top;
	const float Y = Origin.y + R.Top + Top;

	ImGui::PushID(&Splitter);

	const ImVec4 Color = ImGui::GetStyleColorVec4(ImGuiCol_Separator);
	ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, Color);
	ImGui::PushStyleColor(ImGuiCol_SeparatorActive, Color);

	ImGui::SplitterBehavior(ImRect(ImVec2(Origin.x + R.Left, Y - 3), ImVec2(Origin.x + R.Right, Y + 3)), ImGui::GetID("SplitterV"), ImGuiAxis_Y, &Top, &Bottom, 10.0f, 10.0f);

	ImGui::PopStyleColor(2);
	ImGui::PopID();

	Splitter.Ratio = Top / R.GetHeight();
	Splitter.OnResize(R);
}
void FImguiEditorViewportWindow::ShowViewportHorizontalSplitter(SSplitter& Splitter)
{
	const FRect& R = Splitter.Rect;
	const ImVec2 Origin = ImGui::GetMainViewport()->Pos;

	float Left = R.GetWidth() * Splitter.Ratio;
	float Right = R.GetWidth() - Left;
	const float X = Origin.x + R.Left + Left;
	ImGui::PushID(&Splitter);

	const ImVec4 Color = ImGui::GetStyleColorVec4(ImGuiCol_Separator);

	ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, Color);
	ImGui::PushStyleColor(ImGuiCol_SeparatorActive, Color);

	ImGui::SplitterBehavior(ImRect(ImVec2(X - 3, Origin.y + R.Top), ImVec2(X + 3, Origin.y + R.Bottom)), ImGui::GetID("HorizontalSplitter"), ImGuiAxis_X, &Left, &Right, 10.0f, 10.0f);

	ImGui::PopStyleColor(2);
	ImGui::PopID();

	Splitter.Ratio = Left / R.GetWidth();
	Splitter.OnResize(R);
}

bool FImguiEditorViewportWindow::GetViewportSceneRect(
    const ImVec2& Origin,
    FRect& OutRect) const
{
	const ImVec2 ScenePos = ImGui::GetCursorScreenPos();
	const ImVec2 SceneSize = ImGui::GetContentRegionAvail();

	if (SceneSize.x <= 0.0f || SceneSize.y <= 0.0f)
	{
		return false;
	}

	OutRect = FRect{
		ScenePos.x - Origin.x,
		ScenePos.y - Origin.y,
		ScenePos.x - Origin.x + SceneSize.x,
		ScenePos.y - Origin.y + SceneSize.y
	};

	return true;
}

void FImguiEditorViewportWindow::DrawViewportHeader(SEditorViewport& InViewport, FEditor& Editor)
{
	const float HeaderHeight = ImGui::GetFrameHeight();
	const float ButtonSize = HeaderHeight - 6.0f;

	const ImVec4 HeaderColor{ 0.16f, 0.29f, 0.48f, 1.0f };
	const ImVec4 HoverColor{ 0.24f, 0.42f, 0.65f, 1.0f };
	const ImVec4 ActiveColor{ 0.30f, 0.50f, 0.76f, 1.0f };
	const ImVec4 BorderColor{ 0.42f, 0.62f, 0.85f, 1.0f };
	const ImVec4 HighlightColor{ 0.72f, 0.86f, 1.0f, 1.0f };

	ImGui::PushStyleColor(ImGuiCol_ChildBg, HeaderColor);
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, HeaderColor);
	ImGui::PushStyleColor(ImGuiCol_Button, HeaderColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HoverColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ActiveColor);
	ImGui::PushStyleColor(ImGuiCol_Border, BorderColor);
	ImGui::PushStyleColor(ImGuiCol_Header, HoverColor);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HoverColor);
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ActiveColor);

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

	const bool bVisible = ImGui::BeginChild("ViewportHeader", ImVec2(0.0f, HeaderHeight), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	if (bVisible && ImGui::BeginMenuBar())
	{
		ImGui::TextUnformatted("Viewport");

		const ImGuiStyle& Style = ImGui::GetStyle();
		ImDrawList* HeaderDrawList = ImGui::GetWindowDrawList();

		// 오른쪽 끝의 최대화 버튼 위치
		const float ButtonX = ImGui::GetWindowWidth() - Style.WindowPadding.x - ButtonSize;

		// 최대화 버튼 왼쪽의 Camera 메뉴 위치
		const float CameraWidth = ImGui::CalcTextSize("Camera").x + Style.ItemSpacing.x * 3.0f;
		const float CameraX = ButtonX - CameraWidth;

		if (CameraX > ImGui::GetCursorPosX())
			ImGui::SetCursorPosX(CameraX);

		const bool bCameraOpen = ImGui::BeginMenu("Camera");

		// 팝업 내용을 제출하기 전에 메뉴 버튼 정보를 보관
		const ImVec2 CameraMin = ImGui::GetItemRectMin();
		const ImVec2 CameraMax = ImGui::GetItemRectMax();
		const bool bCameraHovered = ImGui::IsItemHovered();

		HeaderDrawList->AddRect(ImVec2(CameraMin.x + 0.5f, CameraMin.y + 0.5f), ImVec2(CameraMax.x - 0.5f, CameraMax.y - 0.5f), ImGui::GetColorU32((bCameraOpen || bCameraHovered) ? HighlightColor : BorderColor), 3.0f, 0, 1.0f);

		if (bCameraOpen)
		{
			SEditorViewport* Viewport = &InViewport;
			FCamera& Camera = Viewport->GetClient().GetViewportCamera();
			ImGui::TextUnformatted("PERSPECTIVE");
			ImGui::Separator();
			if (ImGui::MenuItem("Perspective"))
			{
				if (Camera.GetProjection().GetProjectionType() != EProjectionType::Perspective)
					Camera.SetProjectionType(EProjectionType::Perspective);
				Viewport->GetClient().SetCameraMode(ECameraMode::PERSPECTIVE);
			}
			ImGui::TextUnformatted("ORTHOGRAPHIC");
			ImGui::Separator();
			if (ImGui::MenuItem("Orthographic"))
			{
				if (Camera.GetProjection().GetProjectionType() != EProjectionType::Orthographic)
					Camera.SetProjectionType(EProjectionType::Orthographic);
				Viewport->GetClient().SetCameraMode(ECameraMode::ORTHOGRAPHIC);
			}
			if (ImGui::MenuItem("Top"))
			{
				if (Viewport->GetClient().GetCameraMode() != ECameraMode::ORTHOGRAPHIC_TOP)
					Viewport->GetClient().SetCameraMode(ECameraMode::ORTHOGRAPHIC_TOP);
			}
			if (ImGui::MenuItem("Bottom"))
			{
				if (Viewport->GetClient().GetCameraMode() != ECameraMode::ORTHOGRAPHIC_BOTTOM)
					Viewport->GetClient().SetCameraMode(ECameraMode::ORTHOGRAPHIC_BOTTOM);
			}
			if (ImGui::MenuItem("Left"))
			{
				if (Viewport->GetClient().GetCameraMode() != ECameraMode::ORTHOGRAPHIC_LEFT)
					Viewport->GetClient().SetCameraMode(ECameraMode::ORTHOGRAPHIC_LEFT);
			}
			if (ImGui::MenuItem("Right"))
			{
				if (Viewport->GetClient().GetCameraMode() != ECameraMode::ORTHOGRAPHIC_RIGHT)
					Viewport->GetClient().SetCameraMode(ECameraMode::ORTHOGRAPHIC_RIGHT);
			}
			if (ImGui::MenuItem("Front"))
			{
				if (Viewport->GetClient().GetCameraMode() != ECameraMode::ORTHOGRAPHIC_FRONT)
					Viewport->GetClient().SetCameraMode(ECameraMode::ORTHOGRAPHIC_FRONT);
			}
			if (ImGui::MenuItem("Back"))
			{
				if (Viewport->GetClient().GetCameraMode() != ECameraMode::ORTHOGRAPHIC_BACK)
					Viewport->GetClient().SetCameraMode(ECameraMode::ORTHOGRAPHIC_BACK);
			}
			ImGui::EndMenu();
		}


		// 최대화 버튼 오른쪽 정렬
		if (ButtonX > ImGui::GetCursorPosX())
			ImGui::SetCursorPosX(ButtonX);

		// 줄어든 버튼을 상단바의 세로 중앙에 배치
		ImGui::SetCursorPosY((HeaderHeight - ButtonSize) * 0.5f);

		if (ImGui::Button("##Maximize", ImVec2(ButtonSize, ButtonSize)))
		{
			// 실제 배치 변경은 다음 Process() 시작에서 처리
			PendingMaximizeViewport = &InViewport;
		}

		const ImVec2 ButtonMin = ImGui::GetItemRectMin();
		const ImVec2 ButtonMax = ImGui::GetItemRectMax();
		const bool bButtonHovered = ImGui::IsItemHovered();

		if (bButtonHovered)
		{
			// 호버 시 바깥 테두리 강조
			HeaderDrawList->AddRect(ImVec2(ButtonMin.x + 0.5f, ButtonMin.y + 0.5f), ImVec2(ButtonMax.x - 0.5f, ButtonMax.y - 0.5f), ImGui::GetColorU32(HighlightColor), 3.0f, 0, 1.0f);
			ImGui::SetTooltip("Maximize / Restore");
		}

		ImGui::EndMenuBar();
	}

	ImGui::EndChild();

	ImGui::PopStyleVar(5);
	ImGui::PopStyleColor(9);
}

void FImguiEditorViewportWindow::ApplyPendingViewportMaximize(FEditor& Editor)
{
	if (!PendingMaximizeViewport) { return; }

	SEditorViewport* MaxViewport = PendingMaximizeViewport;
	PendingMaximizeViewport = nullptr;

	Editor.GetViewportLayout().ToggleMaximize(MaxViewport, Editor.State.GetSplitMode());
}
