#include "FRenderView.h"

#include "Editor/Gizmo/FGizmo.h"
#include "Editor/Grid/FGrid.h"
#include "Editor/Visualizer/FVisualizerRegistry.h"
#include "Editor/Visualizer/IVisualizer.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/Actors/AFireBallActor.h"
#include "Runtime/Components/UBillboardComponent.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Components/UFireBallComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Engine/FSceneView.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Engine/FRenderData.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Core/Globals.h"



#include <fstream>

#include "Runtime/CoreUObject/FStatsManager.h"

FRenderView::FRenderView(FRenderer& Renderer) : Renderer(Renderer) {}

namespace
{
	// LODView는 뷰(카메라)당 한 번 만든 값을 넘긴다. 오브젝트마다 카메라 값을 다시 읽지 않기 위함.
	FDrawCommand GetDrawCommand(const UPrimitiveComponent& Component, const FCamera& Camera,
	    const FAxisAlignedBoundingBox& WorldBounds, const UStaticMeshComponent::FLODView& LODView)
	{
		const FRenderData& Data = Component.GetRenderData(Camera);

		if (!Data.Mesh || Data.Materials.empty())
		{
			return {};
		}

		const FMaterialInstance& Material = Data.Materials[0];
		const uint32 LODIndex = UStaticMeshComponent::SelectLOD(Data.Mesh, WorldBounds, LODView);

		// vector에서 span으로 바로 변환하면 ranges 내부 템플릿 호출이 여러 번 생긴다.
		// Instances처럼 포인터와 크기로 직접 만든다.
		const TArray<FMaterial>& CachedMaterials = Component.GetCachedMaterials();

		FDrawCommand Command{
			.Mesh = Data.Mesh->Get(LODIndex),
			.Materials = std::span<const FMaterial>(CachedMaterials.data(), CachedMaterials.size()),
			.Constants = {
			    Material.Color,
			    Material.UVScale,
			    Material.UVOffset,
			    FMatrix::Identity,
			    Material.bDisableShading ? 1.0f : 0.0f,
			},
			.Type = Data.Type,
			.Instances = std::span<const FInstanceData>(Data.Instances.data(), Data.Instances.size()),
			.LODIndex = LODIndex,
		};

		if (Globals::bEnableRenderSort)
		{
			// 같은 애셋이라도 LOD마다 버퍼가 다르므로 LOD 인덱스를 섞는다.
			const uint64 MeshId = static_cast<uint64>(Data.Mesh->GetID().GetHash()) + Data.LODIndex;
			Command.SortKey = Data.SortKey | (MeshId & 0xFFFFull);

			// ============================= Depth 정렬 비활성화 =============================

			// AABB의 Min X 값을 Depth로 지정
			// FAxisAlignedBoundingBox AABB = Component.GetWorldBounds();

			// FVector CameraForward = Camera.GetForwardVector();
			// FVector CameraPosition = Camera.GetPosition();
			// float ProjectedExtent =
			//     std::abs(CameraForward.X) * AABB.Extent.X +
			//     std::abs(CameraForward.Y) * AABB.Extent.Y +
			//     std::abs(CameraForward.Z) * AABB.Extent.Z;

			// Command.Depth = (AABB.Center - CameraPosition).Dot(CameraForward) - ProjectedExtent;
			// float Near = Camera.GetProjection().GetNearPlane();
			// float Far = Camera.GetProjection().GetFarPlane();
			//
			// Command.DepthBucket = static_cast<int32>((Command.Depth - Near) * 32 / (Far - Near));

			// =================================================================================

			Command.Depth = 0.0f;
			Command.DepthBucket = 0;
		}

		return Command;
	}
} // namespace

void FRenderView::CollectScenePrimitives(const ULevel& Scene, const FSceneView& View, const AActor* SelectedActor)
{
	const TArray<UPrimitiveComponent*>& Primitives = Scene.GetRenderComponents();
	// Primitives[i]의 SceneIndex는 i이므로 CullDataList[i]가 그 컴포넌트의 월드 바운드다 (VisibleFlags와 같은 규칙)
	const TArray<FAxisAlignedBoundingBox>& CullDataList = Scene.GetCullDataList();
	const UStaticMeshComponent::FLODView LODView = UStaticMeshComponent::MakeLODView(View.Camera);
	RenderQueue.Reserve(Primitives.size());

	std::fill(std::begin(Globals::LODDrawCounts), std::end(Globals::LODDrawCounts), 0u);

	// assert(!bCullResultValid || VisibleFlags.size() == Primitives.size());

	// 컬링 결과 인덱스를 맞추기 위해 인덱스 for문으로 변경
	for (size_t i = 0; i < Primitives.size(); i++)
	{
		// 컬링을 사용중인데 컬링 되어 버렸다면
		const bool bCulled = bCullResultValid && !VisibleFlags[i];
		const bool bOccluded = i < OccludedFlags.size() && OccludedFlags[i];

		// 오라클 프레임: 오클루전으로 지운 것은 검증을 위해 명령만 만들고 그리지 않는다
		// bOracleRequested가 true여야 한다.
		const bool bCollectForOracleOnly = bCulled && bOccluded && bOracleRequested;

		// 컬링 되었는데 오라클을 사용하지 않는다면 조기 종료. 오라클을 사용한다면 진행한다.
		if (bCulled && !bCollectForOracleOnly)
			continue;

		UPrimitiveComponent* PrimitiveComponent = Primitives[i];
		if (!PrimitiveComponent)
			continue;

		// bCullResultValid가 false라면 통과
		//  bCullResultValid가 true라면 컬링 결과 통과시에만 수집
		// if (bCullResultValid && !VisibleFlags[i]) continue;

		// 쇼 플래그 확인
		if ((static_cast<uint64>(View.ShowFlags) & static_cast<uint64>(PrimitiveComponent->GetShowFlag())) == 0)
		{
			continue;
		}

		bool bSelected = false;
		if (PrimitiveComponent->GetActorOwner() && PrimitiveComponent->GetActorOwner() == SelectedActor)
		{
			bSelected = true;
		}

		FDrawCommand DrawCommand = GetDrawCommand(*PrimitiveComponent, View.Camera, CullDataList[i], LODView);

		// 인스턴싱 및 텍스트는 인스턴스 배열을 사용하므로 바로 푸시
		if (DrawCommand.Type == ERenderType::Text || DrawCommand.Type == ERenderType::Instancing)
		{
			if (bCollectForOracleOnly)
				continue;
			RenderQueue.Push(std::move(DrawCommand));
			continue;
		}

		int32 Index = PrimitiveComponent->GetBatchIndex();

		if (!PrimitiveComponent->Cast<UBillboardComponent>())
		{
			DrawCommand.Constants.World = PrimitiveComponent->GetGlobalTransformMatrix();
		}
		else
		{
			const FMatrix World = PrimitiveComponent->GetRenderMatrix(View.Camera);
			DrawCommand.Constants.World = World;
		}
		FMatrix InverseWorld;
		if (DrawCommand.Constants.World.Inverse(InverseWorld))
		{
			DrawCommand.Constants.WorldInverseTranspose = InverseWorld.Transpose();
		}
		else
		{
			DrawCommand.Constants.WorldInverseTranspose = FMatrix::Identity;
		}
		DrawCommand.Constants.DisableShading = View.ViewMode == EViewModeIndex::VMI_Unlit ? 1.0f : 0.0f;

		AActor* Owner = PrimitiveComponent->GetActorOwner();
		AFireBallActor* FireBallActor = Owner ? Owner->Cast<AFireBallActor>() : nullptr;
		UFireBallComponent* FireBall =
		    FireBallActor && PrimitiveComponent == FireBallActor->GetSphereComponent()
		        ? FireBallActor->GetFireBallComponent()
		        : nullptr;

		if (FireBall)
		{
			DrawCommand.Constants.EmissiveColor = FireBall->GetEmissiveColor();
			DrawCommand.Constants.EmissiveIntensity = FireBall->GetEmissiveIntensity();
		}

		if (DrawCommand.Mesh)
		{
			const uint32 DebugLOD = std::min(DrawCommand.LODIndex, Globals::MaxDebugLODCount - 1);
			++Globals::LODDrawCounts[DebugLOD];

			if (Globals::bShowLODColor)
			{
				// 언리얼의 LOD Coloration과 같은 순서: 흰색, 빨강, 초록, 파랑
				static const FVector4 LODColors[Globals::MaxDebugLODCount]{
					{ 1.0f, 1.0f, 1.0f, 0.8f },
					{ 1.0f, 0.2f, 0.2f, 0.8f },
					{ 0.2f, 1.0f, 0.2f, 0.8f },
					{ 0.2f, 0.4f, 1.0f, 0.8f },
				};
				DrawCommand.Constants.Color = LODColors[DebugLOD];
			}
		}

		if (bCollectForOracleOnly)
		{
			// 지운 것
			OracleOccludedCommands.push_back(DrawCommand);
			continue;
		}
		if (bOracleRequested && DrawCommand.Type == ERenderType::Primitive)
		{
			// 그린것
			OracleDrawnCommands.push_back(DrawCommand);
		}

		RenderQueue.Push(std::move(DrawCommand));
	}
}

void FRenderView::PrepareRender()
{
	FFrameConstants FrameConstants{
		.Time = FTimeManager::GetTime(),
		.DeltaTime = FTimeManager::GetDeltaTime(),
	};

	Renderer.UpdateFrameConstants(FrameConstants);
}

void FRenderView::RenderView(const FSceneView& View, const ULevel& Scene, const FEditorRenderContext& EditorCtx)
{
	// 뷰포트 시작

	// 이 뷰 크기의 SceneTextures를 확보하지 못했으면 그리지 않는다
	if (!BeginView(View))
	{
		return;
	}

	// 컬링 측정
	{
		CullScene(View, Scene);
	}

	// 씬 컴포넌트 수집 (LOD 선택 포함). 독립 카운터라 부모인 Draw 수치에는 영향이 없다.
	{
		SCOPE_CYCLE_COUNTER_IMPL(__COUNTER__, "Collect", true);
		CollectScenePrimitives(Scene, View, EditorCtx.SelectedActor);
	}

	if (Globals::bEnableRenderSort)
	{
		RenderQueue.Sort();
	}

	TArray<FPointLightConstants> PointLights;
	CollectPointLights(Scene, PointLights);
	Renderer.UploadPointLights(PointLights);
	Renderer.BindPointLights();

	// 기본 씬 오브젝트 패스
	FlushBasePass(View);

	// BasePass 이후에 깊이 버퍼 기준으로 가시성 질의
	if (bOracleRequested)
	{
		RunOcclusionOracle();
		bOracleRequested = false;
	}

	Renderer.ClearLastRenderState();

	// 에디터 라인 패스
	if (EditorCtx.Grid && (View.ShowFlags & static_cast<uint32>(EEngineShowFlags::SF_Grid)) != 0)
	{
		DrawGrid(View.Camera, *EditorCtx.Grid);
	}

	if (EditorCtx.SelectedPrimitive && EditorCtx.VisualizerRegistry)
	{

		UClass* ClassType = EditorCtx.SelectedPrimitive->GetClass();
		FVisualizerRegistry& Registry = *EditorCtx.VisualizerRegistry;

		IVisualizer* Visualizer = Registry.FindVisualizer(ClassType);

		if (Visualizer)
		{
			Visualizer->Draw(*EditorCtx.SelectedPrimitive, *this, View.Camera, FVector4{ 0.0f, 1.0f, 0.0f, 1.0f });
		}
	}

	// Post Process 패스
	RenderPostProcessPass(View, EditorCtx.SelectedActor);
	Renderer.ClearLastRenderState();

	FlushLinePass(View.Camera);
	Renderer.ClearLastRenderState();
}

// 뷰포트 렌더 시작시 실행하는 것들
bool FRenderView::BeginView(const FSceneView& View)
{
	// 이 뷰포트와 같은 크기의 SceneTextures를 요청
	FSceneTextures* SceneTextures = Renderer.AcquireSceneTextures(static_cast<UINT>(View.ViewportSizePixel.X), static_cast<UINT>(View.ViewportSizePixel.Y));

	// 실패 시 이 뷰포트는 그리지 않음
	if (!SceneTextures)
	{
		return false;
	}

	// SceneColor + SceneDepth 바인딩
	Renderer.BindRenderTarget(SceneTextures->GetCurrentRTV(), SceneTextures->SceneDepthDSV.Get());
	Renderer.SetViewportPixel(View.ViewportSizePixel);
	Renderer.UpdateLightConstants(View.LightConstants);

	// SceneTextures(공용 도화지)를 그리기 전에 Clear
	Renderer.GetContext()->ClearRenderTargetView(SceneTextures->GetCurrentRTV(), ClearColor);
	Renderer.GetContext()->ClearDepthStencilView(SceneTextures->SceneDepthDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ViewConstants 갱신
	UpdateViewConstants(View.Camera, View.ViewportSizePixel);
	return true;
}

void FRenderView::UpdateViewConstants(const FCamera& Camera, FVector2 ViewportSizePixel)
{
	FMatrix Projection = Camera.GetProjectionMatrix();
	FMatrix ProjectionD3D = Projection.ToD3DMatrix();
	FMatrix ViewProjectionD3D = Camera.GetViewMatrix() * ProjectionD3D;
	FMatrix InverseVPD3D;
	ViewProjectionD3D.Inverse(InverseVPD3D);

	FViewConstants ViewConstants{
		.View = Camera.GetViewMatrix(),
		.Projection = ProjectionD3D,
		.ViewProjectionInverse = InverseVPD3D,
		.ViewportSize = ViewportSizePixel,
		.NearZ = Camera.GetProjection().GetNearPlane(),
		.FarZ = Camera.GetProjection().GetFarPlane(),
		.IsPerspective = Camera.GetProjection().GetProjectionType() == EProjectionType::Perspective ? 1.f : 0.f,
		.CameraPos = Camera.GetPosition(),
	};

	Renderer.UpdateViewConstants(ViewConstants);
}

void FRenderView::DrawGrid(const FCamera& Camera, FGrid& Grid)
{
	Grid.DrawLine(Renderer, Camera);

	FGridLineConstants Constants{};
	Constants.MVP = Camera.GetViewProjectionMatrix();
	Constants.CameraPosition = Camera.GetPosition();
	Constants.FadeStartDistance = 3.0f;
	Constants.FadeEndDistance = 75.0f;
	Renderer.FlushLineBatch(Constants, FName("Grid"));
}

void FRenderView::FlushBasePass(const FSceneView& View)
{
	FlushQueue(View);
}

void FRenderView::FlushLinePass(const FCamera& Camera)
{
	FlushLineBatch(Camera.GetViewProjectionMatrix());
}

// Overlay 되는 것 그리는 Pass (지금은 기즈모, 텍스트)
void FRenderView::RenderOverlayPass(const FSceneView& View, const FTransform& SelectedTransform, const FGizmo& Gizmo, UTextComponent* TextComp)
{
	// 출력 RT(뷰포트 크기) + SceneDepthDSV(같은 크기)를 함께 바인딩. 
	const FViewportRenderTarget* RenderTarget = View.Viewport.RenderTarget.get();
	FSceneTextures* SceneTextures = Renderer.GetSceneTextures();

	// 둘 중 하나라도 없으면 그리지 않는다
	if (!RenderTarget || !SceneTextures) 
	{
		return;
	}
	Renderer.BindRenderTarget(RenderTarget->GetRTV(), SceneTextures->SceneDepthDSV.Get());

	// 뷰포트 크기 재설정
	Renderer.SetViewportPixel(View.ViewportSizePixel);
	UpdateViewConstants(View.Camera, View.ViewportSizePixel);

	// 텍스트 오버레이 렌더링
	if (TextComp && (View.ShowFlags & static_cast<uint64>(EEngineShowFlags::SF_BillboardText)))
	{
		Renderer.ClearDepth();
		FDrawCommand Command = GetDrawCommand(*TextComp, View.Camera, TextComp->GetWorldBounds(), UStaticMeshComponent::MakeLODView(View.Camera));
		if (!Command.Instances.empty())
		{
			Renderer.AddTextInstanceArray(Command);
			Renderer.DrawTextInstances(Command);
			Renderer.ClearTextInstances();
		}
	}
}

void FRenderView::RenderGizmo(const FSceneView& View, const FTransform& Transform, const FGizmo& Gizmo)
{
	const FViewportRenderTarget* RenderTarget = View.Viewport.RenderTarget.get();
	FSceneTextures* SceneTextures = Renderer.GetSceneTextures();
	if (!RenderTarget || !SceneTextures)
	{
		return;
	}
	Renderer.BindRenderTarget(RenderTarget->GetRTV(), SceneTextures->SceneDepthDSV.Get());

	Renderer.SetViewportPixel(View.ViewportSizePixel);
	UpdateViewConstants(View.Camera, View.ViewportSizePixel);
	Renderer.ClearDepth();
	Gizmo.Draw(Renderer, Transform, View.Camera);
}

void FRenderView::RenderLine(const FVector& Start, const FVector& End,
    const FVector4& Color)
{
	FLineBatcher& LineBatcher = Renderer.GetLineBatcher();
	LineBatcher.DrawLine(Start, End, Color);
}

void FRenderView::RenderBoxCenterExtent(const FVector& Center, const FVector& Extent, const FVector4& Color)
{
	FLineBatcher& LineBatcher = Renderer.GetLineBatcher();
	LineBatcher.DrawBoxCenterExtent(Center, Extent, Color);
}

void FRenderView::RenderBoxMinMax(const FVector& Min, const FVector& Max, const FVector4& Color)
{
	FLineBatcher& LineBatcher = Renderer.GetLineBatcher();
	LineBatcher.DrawBoxMinMax(Min, Max, Color);
}

void FRenderView::RenderQuad(
    const FVector& A,
    const FVector& B,
    const FVector& C,
    const FVector& D,
    const FVector4& Color)
{
	FLineBatcher& LineBatcher = Renderer.GetLineBatcher();
	LineBatcher.DrawQuad(A, B, C, D, Color);
}

void FRenderView::RenderSphere(const FVector& Center, float Radius, const FVector4& Color, uint32 Segments)
{
	FLineBatcher& LineBatcher = Renderer.GetLineBatcher();
	LineBatcher.DrawSphere(Center, Radius, Color, Segments);
}

void FRenderView::DrawStencilMask(const FCamera& Camera, const AActor* SelectedActor)
{
	if (!SelectedActor) return;

	USceneComponent* RootComp = SelectedActor->GetRootComponent();
	if (!RootComp) return;

	UPrimitiveComponent* PrimComp = RootComp->Cast<UPrimitiveComponent>();
	if (!PrimComp) return;

	FDrawCommand DrawCommand = GetDrawCommand(*PrimComp, Camera, PrimComp->GetWorldBounds(), UStaticMeshComponent::MakeLODView(Camera));

	const FMatrix ModelMatrix = PrimComp->GetRenderMatrix(Camera);
	
	DrawCommand.Constants.World = ModelMatrix;
	DrawCommand.Constants.DisableShading = true;

	auto SelectionStencilMaterial = FRenderResourceLibrary::Get().GetMaterial("#SelectionStencil");
	if (SelectionStencilMaterial)
	{
		SelectionStencilMaterial->GetPipeline()->SetStencilRef(1);
		DrawCommand.Materials = std::span<const FMaterial>(SelectionStencilMaterial.get(), 1);
		Renderer.Draw(DrawCommand, nullptr, 2);
	}
}

// Post Process Pass에서 렌더할 것들
// 어느 분기든 마지막 패스는 반드시 뷰포트 출력 RT 전체를 써야한다
void FRenderView::RenderPostProcessPass(const FSceneView& View, const AActor* SelectedActor)
{
	// 스텐실 마스크는 이전에 바인딩 된 DSV에 스텐실을 써야하기 때문에 첫번째로 실행
	DrawStencilMask(View.Camera, SelectedActor);

	FFogSettings FogSettings = GFogSettings; // ImGui에서 조절한 값

	// PostProcess 상수버퍼
	FPostProcessConstants Constants = {
		.VisMax = 10.f,
		.VisMinOrtho = 0.1f,
		.VisMaxOrtho = 10.f,
		.FogHeightFalloff = FogSettings.FogHeightFalloff,
		.CameraHeightDensity = FogSettings.GetCameraHeightDensity(View.Camera.GetPosition().Z, FogSettings.FogHeight),
		.StartDistance = FogSettings.StartDistance,
		.FogCutoffDistance = FogSettings.FogCutoffDistance,
		.FogMaxOpacity = FogSettings.FogMaxOpacity,
		.FogInscatteringColor = FogSettings.FogInscatteringColor,
	};
	Renderer.UpdatePostProcessConstants(Constants); // 설정값 (상수버퍼) 업데이트

	// Scene Depth 모드
	if (View.ViewMode == EViewModeIndex::VMI_SceneDepth)
	{
		Renderer.RenderSceneDepth(View.Viewport);
		return; // Scene Depth만 그린다
	}
	
	// Fog를 그린다
	Renderer.RenderFog(View.Viewport);

	// DrawStencilMask에서 쓴 스텐실 대로 아웃라인을 그린다 
	Renderer.RenderSelectionOutline(View.Viewport);
}

void FRenderView::DrawInstances(const FSceneView& View, FRenderPipeline* Pipeline)
{
	Renderer.DrawInstances(View.Camera, Pipeline, View.ViewMode == EViewModeIndex::VMI_Unlit);
}

void FRenderView::FlushLineBatch(const FMatrix& ViewProjection, const FName& PipelineId)
{
	FObjectConstants Constants{};
	Constants.World = FMatrix::Identity;
	Constants.DisableShading = 1.0f;
	Renderer.FlushLineBatch(Constants, PipelineId);
}

void FRenderView::FlushQueue(const FSceneView& View)
{
	auto& ResLib = FRenderResourceLibrary::Get();

	// 와이어 프레임인 경우 와이어 프레임 파이프라인을 쓴다
	FRenderPipeline* OverridePipeline = nullptr;
	if (View.ViewMode == EViewModeIndex::VMI_Wireframe)
	{
		OverridePipeline = ResLib.GetPipeline(FName("#Simple_Wireframe")).get();
	}

	// Primitive 큐 처리
	Renderer.DrawPrimitiveBatch(RenderQueue.GetPrimRenderQ(), OverridePipeline);

	// Instancing 큐
	if (!RenderQueue.IsInstancingRQEmpty())
	{
		for (const FDrawCommand& Data : RenderQueue.GetInstancingRenderQ())
		{
			Renderer.AddTextInstanceArray(Data);
		}
		DrawInstances(View, OverridePipeline);

		Renderer.ClearTextInstances();
	}

	// Spotlight 큐: 불투명 렌더링 후 가산 블렌딩 수행
	for (const FDrawCommand& Data : RenderQueue.GetSpotlightRenderQ())
	{
		Renderer.Draw(Data, OverridePipeline);
	}

	// Text 큐: BuildRenderData()에서 이미 계산된 Instances 배열 사용
	if (!RenderQueue.IsTextRQEmpty())
	{
		for (const FDrawCommand& Data : RenderQueue.GetTextRenderQ())
		{
			// Font에서 미리 계산된 글자별 쿼드 데이터를 그대로 넘김
			Renderer.AddTextInstanceArray(Data);
		}

		// 각 DrawCommand의 머티리얼 주소가 인스턴스 배치 키에 포함되므로,
		// 첫 번째 명령만 그리면 나머지 Text 배치는 렌더링되지 않는다.
		for (const FDrawCommand& Data : RenderQueue.GetTextRenderQ())
		{
			Renderer.DrawTextInstances(Data);
		}
		Renderer.ClearTextInstances();
	}

	RenderQueue.Clear();
}

void FRenderView::CollectPointLights(const ULevel& Scene, TArray<FPointLightConstants>& OutLights)
{
	OutLights.clear();

	for (UPrimitiveComponent* Component : Scene.GetRenderComponents())
	{
		if (!Component)
		{
			continue;
		}

		UFireBallComponent* Light = Component->Cast<UFireBallComponent>();

		if (Light)
		{
			OutLights.push_back(Light->GetPointLightData());
		}
	}
}

FCullingSettings& FRenderView::GetCullingSettings()
{
	return CullingSettings;
}

const FCullingSettings& FRenderView::GetCullingSettings() const
{
	return CullingSettings;
}

void FRenderView::SetCullingEnabled(bool pCullingEnable)
{
	Globals::bEnableFrustumCulling = pCullingEnable;
}

void FRenderView::CullScene(const FSceneView& View, const ULevel& Scene)
{
	const TArray<FAxisAlignedBoundingBox>& CullDataList = Scene.GetCullDataList();

	const bool bUseFrustum = Globals::bEnableFrustumCulling;

	// 와이어 프레임일때는 뒤가 비쳐 보이니 오클루전을 쓰지 않도록 한다.
	const bool bUseOcclusion = Globals::bEnableOcclusionCulling && View.ViewMode != EViewModeIndex::VMI_Wireframe;

	if (Globals::bRequestOcclusionOracle)
	{
		bOracleRequested = true;
		Globals::bRequestOcclusionOracle = false; // 다음에 렌더되는 뷰 하나만
	}

	OccludedFlags.clear();
	Globals::OccludedCount = 0;
	bCullResultValid = bUseFrustum || bUseOcclusion;

	// 컬링하지 않는다면 종료
	if (!bCullResultValid)
	{
		Globals::FrustumVisibleCount = static_cast<uint32>(CullDataList.size());
		return;
	}

	if (bUseFrustum)
	{
		SCOPE_CYCLE_COUNTER("Frustum");
		// 매 프레임 그 프레임의 Frustum으로 전체 판정 (이전 결과 재사용 없음)
		const FFrustum Frustum = GetCullFrustum(View);
		Globals::FrustumVisibleCount = Culler->Cull(Frustum, CullDataList, VisibleFlags);
	}
	else
	{
		// Frustum 없이 오클루전만
		VisibleFlags.assign(CullDataList.size(), 1);
		Globals::FrustumVisibleCount = static_cast<uint32>(CullDataList.size());
	}

	if (bUseOcclusion)
	{
		// ImGui 값을 컬러에 반영
		OcclusionCuller.OccluderBudget = static_cast<uint32>(std::max(0, Globals::OccluderBudget));
		OcclusionCuller.BufferWidth = std::max(16, Globals::OcclusionBufferWidth);
		OcclusionCuller.bIncludeOccluderCull = Globals::bIncludeOccluderCull;
		if (Globals::bRequestOcclusionDump)
		{
			OcclusionCuller.bDumpNextFrame = true;
			Globals::bRequestOcclusionDump = false;
		}

		SCOPE_CYCLE_COUNTER("Occlusion");
		Globals::OccludedCount = OcclusionCuller.Cull(View, Scene, VisibleFlags, OccludedFlags);
	}
}

FFrustum FRenderView::GetCullFrustum(const FSceneView& View)
{
	return FFrustum::FromViewProjection(View.ViewProj);
}

void FRenderView::RunOcclusionOracle()
{
	TArray<const FDrawCommand*> Commands;
	Commands.reserve(OracleDrawnCommands.size() + OracleOccludedCommands.size());
	for (const FDrawCommand& Command : OracleDrawnCommands)
	{
		Commands.push_back(&Command);
	}
	for (const FDrawCommand& Command : OracleOccludedCommands)
	{
		Commands.push_back(&Command);
	}

	TArray<uint64> Samples;
	Renderer.QueryVisibility(Commands, Samples);

	const size_t DrawnCount = OracleDrawnCommands.size();
	const size_t OccludedCount = OracleOccludedCommands.size();

	uint32 DrawnVisible = 0; // 그렸고 실제로 보임
	uint32 Violations = 0;   // 오클루전으로 지웠는데 실제로는 보임 (버그)
	for (size_t i = 0; i < Samples.size(); ++i)
	{
		const bool bVisible = Samples[i] > 0;
		if (i < DrawnCount)
		{
			DrawnVisible += bVisible ? 1 : 0;
		}
		else
		{
			Violations += bVisible ? 1 : 0;
		}
	}

	const uint32 Total = static_cast<uint32>(DrawnCount + OccludedCount); // Frustum 통과 메시 수
	const uint32 DrawnButHidden = static_cast<uint32>(DrawnCount) - DrawnVisible;
	const uint32 TrulyHidden = DrawnButHidden + (static_cast<uint32>(OccludedCount) - Violations);
	const float MaxRatio = Total ? 100.0f * TrulyHidden / Total : 0.0f;
	const float Achieved = TrulyHidden ? 100.0f * (OccludedCount - Violations) / TrulyHidden : 0.0f;

	UE_LOG("[Oracle] 대상 %u | 그림 %zu (실제 보임 %u, 가려졌는데 그림 %u) | 오클루전 컬링 %zu (위반 %u)",
	    Total, DrawnCount, DrawnVisible, DrawnButHidden, OccludedCount, Violations);
	UE_LOG("[Oracle] 이론적 최대 컬링 %u개 (%.1f%%) | 현재 달성률 %.1f%%",
	    TrulyHidden, MaxRatio, Achieved);
	if (Violations > 0)
	{
		UE_LOG_ERROR("[Oracle] 보이는 오브젝트 %u개를 지웠습니다. 보수성 버그", Violations);
	}

	OracleDrawnCommands.clear();
	OracleOccludedCommands.clear();
}
