#pragma once

#include "FMaterial.h"
#include "FMesh.h"
#include "FRenderPipeline.h"
#include "FRenderResourceLibrary.h"
#include "FViewportRenderTarget.h"
#include "FSceneTextures.h"
#include "ShaderConstants.h"
#include "Vertices.h"

#include "Runtime/Engine/Types/IntTypes.h"
#include "Runtime/Engine/Types/PointerTypes.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/Core/FRect.h"
#include "Runtime/CoreUObject/FStatsManager.h"
#include "Runtime/Engine/FViewport.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Material/FTextureSamplerDesc.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Rendering/FLineBatcher.h"

#include <Windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <wrl/client.h>
#include <span>
#include <type_traits>
#include <utility>

class FTexture;
struct FTextureDesc;
class FCamera;
struct FDrawCommand;
struct FPointLightConstants;
struct FSceneView;

struct FFrameResource
{
	Microsoft::WRL::ComPtr<ID3D11Buffer> FrameConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> ViewConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> ObjectConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> PostProcessConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> HeightFogConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> FXAAConstantBuffer;
};

inline float ClearColor[4] = { 0.6f, 0.75f, 1.0f, 1.0f };

class FRenderer final
{
public:
	void DrawBillboardText(const FSceneView& View, FWStringView Text,
	    const FVector& WorldPosition, float WorldSize);
	// 생명주기
	bool Initialize(HWND Window);
	void Shutdown();
	void OnWindowSize(UINT Width, UINT Height);

	// 프레임 진행 / 렌더 타깃 / 뷰포트
	void BeginFrame();
	void BindRenderTarget(ID3D11RenderTargetView* RTV, ID3D11DepthStencilView* DSV);
	void SetViewportPixel(FVector2 ViewportSizePixel);
	void ClearDepth();
	void SwapBuffer();
	void FlushDrawStats();
	void ClearLastRenderState();

	bool PrepareViewportRenderTarget(FViewport& InViewport);
	FSceneTextures* AcquireSceneTextures(UINT Width, UINT Height);
	FSceneTextures* GetSceneTextures() { return ActiveSceneTextures; }
	void CompositeViewport(const FViewport& InViewport);

	// 접근자
	void GetDeviceAndContext_ImplDX11(ID3D11Device*& DeviceOut, ID3D11DeviceContext*& ContextOut);
	ID3D11Device* GetDevice() const { return Device.Get(); }
	ID3D11DeviceContext* GetContext() const { return Context.Get(); }

	ID3D11RenderTargetView* GetBackBufferRTV() { return BackBufferRTV.Get(); }

	float GetWidth() const { return Viewport.Width; }
	float GetHeight() const { return Viewport.Height; }

	FLineBatcher& GetLineBatcher() { return LineBatcher; }

	// 리소스 생성 / 조회
	TSharedPtr<FMesh> CreateMesh(const FMeshDesc& Desc);
	TSharedPtr<FMesh> CreateDynamicMesh(const FMeshDesc& Desc); 
	TSharedPtr<FRenderPipeline> CreateRenderPipeline(const FRenderPipelineDesc& Desc);
	TSharedPtr<FTexture> CreateTexture(const wchar_t* path);
	TSharedPtr<FTexture> CreateSolidTexture(const FVector4& Color);
	TSharedPtr<FRenderPipeline> GetPipeline(const FName& Id) const;

	// 상수 버퍼 갱신
	void UpdateLightConstants(const FLightConstants& Constants);
	void UploadPointLights(std::span<const FPointLightConstants> PointLights);
	void BindPointLights();
	void UpdateFrameConstants(const FFrameConstants& Constants);
	void UpdateViewConstants(const FViewConstants& Constants);
	// 카메라로부터 ViewConstants를 만들어 갱신한다 (Projection은 D3D Clip 기준)
	void UpdateViewConstants(const FCamera& Camera, FVector2 ViewportSizePixel);
	void UpdatePostProcessConstants(const FPostProcessConstants& Constants);
	void UpdateHeightFogConstants(const FHeightFogConstants& Constants);
	void UpdateFXAAConstants(const FFXAAConstants& Constants);

	// Object Constant Buffer를 갱신한다.
	template <typename TConstants>
	void UpdateBuffer(const TConstants& Constants, uint32 Slot)
	{
		static_assert(sizeof(TConstants) <= ConstantBufferSize);
		static_assert(sizeof(TConstants) % 16 == 0);

		// 언리얼 Clip -> D3D Clip 좌표 변환.
		// Object MVP는 현재 뷰와 World로 계산하고, 그리드 MVP는 전달된 값을 사용한다.
		TConstants ShaderConstants = Constants;
		//if constexpr (std::is_same_v<TConstants, FObjectConstants>)
		//{
		//	ShaderConstants.MVP = ShaderConstants.World * ViewProjection;
		//}
		//if constexpr (requires { ShaderConstants.MVP; })
		//{
		//	ShaderConstants.MVP = ShaderConstants.MVP.ToD3DMatrix();
		//}

		ID3D11Buffer* ObjectCB = GetCurrentFrameResource()->ObjectConstantBuffer.Get();

		D3D11_MAPPED_SUBRESOURCE Mapped{};
		if (FAILED(Context->Map(ObjectCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
		{
			return;
		}
		std::memcpy(Mapped.pData, &ShaderConstants, sizeof(TConstants));
		Context->Unmap(ObjectCB, 0);

		Context->VSSetConstantBuffers(Slot, 1u, &ObjectCB);
		Context->PSSetConstantBuffers(Slot, 1u, &ObjectCB);
	}

	// 드로우 
	void Draw(const FDrawCommand& Command, FRenderPipeline* OverridePipeline, uint32 Slot = 2);

	template <typename TConstants>
	void Draw(const FMesh& Mesh, const FMaterial& Material, const TConstants& Constants, FRenderPipeline* OverridePipeline, uint32 Slot = 2)
	{
		UpdateBuffer(Constants, 2);

		BindDrawResources(Mesh, Material, OverridePipeline);

		if (Mesh.HasIndices())
		{
			Context->DrawIndexed(Mesh.IndexCount, 0, 0);
			PendingPrimCount += Mesh.IndexCount / 3u;
		}
		else
		{
			Context->Draw(Mesh.VertexCount, 0);
			PendingPrimCount += Mesh.VertexCount / 3u;
		}
		++PendingDrawCount;
	}

	template <typename TConstants>
	void DrawSection(const FMesh& Mesh, const FMaterial& Material, const TConstants& Constants, FRenderPipeline* OverridePipeline, uint32 StartIndex, uint32 IndexCount, uint32 Slot = 2)
	{
		UpdateBuffer(Constants, Slot);

		BindDrawResources(Mesh, Material, OverridePipeline);

		if (Mesh.HasIndices())
		{
			Context->DrawIndexed(IndexCount, StartIndex, 0);
			PendingPrimCount += IndexCount / 3u;
		}
		else
		{
			Context->Draw(Mesh.VertexCount, 0);
			PendingPrimCount += Mesh.VertexCount / 3u;
		}
		++PendingDrawCount;
	}

	void BindDrawResources(const FMesh& Mesh, const FMaterial& Material, FRenderPipeline* OverridePipeline);

	void DrawPrimitiveBatch(std::span<const FDrawCommand> Commands, FRenderPipeline* OverridePipeline);
	bool UploadObjectConstants(std::span<const FDrawCommand> Commands);
	void BindObjectConstantRange(uint32 Slot, uint32 ByteOffset);
	void DrawUploadedCommand(const FDrawCommand& Command, FRenderPipeline* OverridePipeline);

	// 텍스트 인스턴싱
	void AddTextInstanceArray(const FDrawCommand& Command);
	void DrawInstances(const FCamera& Camera, FRenderPipeline* OverridePipeline, bool bDisableShading);
	void DrawTextInstances(const FDrawCommand& Command);
	void ClearTextInstances();

	// 라인 배치
	template <typename TConstants>
	void FlushLineBatch(const TConstants& Constants, const FName& PipelineId = FName("#Simple_Line"))
	{
		UpdateBuffer(Constants, 2);
		LineBatcher.Flush(*Context.Get(), GetPipeline(PipelineId));
	}

	// 스크린 패스 / 후처리
	void DrawScreenPass(ID3D11RenderTargetView* TargetRTV, const D3D11_VIEWPORT& TargetD3DViewport, ID3D11ShaderResourceView* const* SRVs, UINT NumSRVs, const FName& PipelineId);
	void CopySceneColorToViewport(const FViewport& TargetViewport);
	void RenderSelectionOutline(const FViewport& TargetViewport);
	void RenderSceneDepth(const FViewport& TargetViewport);
	void RenderFog(const FViewport& TargetViewport);
	void RenderFXAA(ID3D11ShaderResourceView* InputSRV, ID3D11RenderTargetView* OutputRTV, UINT Width, UINT Height);
	void RenderViewportFXAA(const FViewport& TargetViewport);

	// 디버그
	void QueryVisibility(const TArray<const FDrawCommand*>& Commands, TArray<uint64>& OutSamples);

private:
	// 초기화
	void DrawTextInstanceData(const FDrawCommand& Command, std::span<const FInstanceData> InstanceData);
	bool InitializeDeviceAndSwapChain(HWND Window);
	bool InitializeBackBuffer();
	bool InitializeConstantBuffers();
	bool InitializeGPUTimerQueries();

	// GPU 타이머
	void BeginGPUTimer();
	void EndGPUTimer();
	void ResolveGPUTimer();

	// 파이프라인 상태 캐시
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> GetOrCreateRasterizerState(const FRasterizerDesc& Desc);
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> GetOrCreateDepthStencilState(const FDepthStencilDesc& Desc);
	Microsoft::WRL::ComPtr<ID3D11BlendState> GetOrCreateBlendState(const FBlendDesc& Desc);
	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetOrCreateSamplerState(const FTextureSamplerDesc& Desc);

	// 프레임 리소스 링
	FFrameResource* GetCurrentFrameResource() { return &FrameResources[CurrentFrameResourceIndex]; }
	FFrameResource* GetNextFrameResource() { return &FrameResources[(CurrentFrameResourceIndex + 1) % NumFrameResourceCount]; }


	D3D11_VIEWPORT MakeD3DViewport(float Left, float Top, float Width, float Height) const;

	void EvictUnusedSceneTextures();

	bool InitializePointLightBuffers();


private:
	// 모든 ConstantBuffer의 최대 크기
	static constexpr UINT ConstantBufferSize = 256u;
	static constexpr uint32 GPUTimerFrameCount = 3u;
	static constexpr uint32 NumFrameResourceCount = 5;

	FLineBatcher LineBatcher;
	FMatrix ViewProjection = FMatrix::Identity;

	// 디바이스
	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;
	D3D11_VIEWPORT Viewport{};
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> Context1; // D3D11_1 Extension

	// 상수 버퍼 (Frame/View/Object는 FrameResources에 있음)
	Microsoft::WRL::ComPtr<ID3D11Buffer> LightConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> ObjectConstantUploadBuffer; // 임시 상수버퍼
	Microsoft::WRL::ComPtr<ID3D11Buffer> PointLightBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> PointLightCountBuffer;

	// 포인트 라이트
	static constexpr uint32 MaxPointLightCount = 64;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> PointLightSRV;

	// Draw, ImGui 모두 다 포함하는 BackBuffer Texture
	Microsoft::WRL::ComPtr<ID3D11Texture2D> BackBufferTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> BackBufferRTV;

	// SceneTextures
	TMap<std::pair<UINT, UINT>, FSceneTextures, FPairHash> SceneTexturesPool;
	FSceneTextures* ActiveSceneTextures = nullptr;

	uint64 FrameCounter = 0;
	static constexpr uint64 SceneTexturesKeepFrames = 2;

	// 파이프라인 상태 캐시
	TMap<FRasterizerDesc, Microsoft::WRL::ComPtr<ID3D11RasterizerState>> RasterizerStateMap;
	TMap<FDepthStencilDesc, Microsoft::WRL::ComPtr<ID3D11DepthStencilState>> DepthStencilStateMap;
	TMap<FBlendDesc, Microsoft::WRL::ComPtr<ID3D11BlendState>> BlendStateMap;
	TMap<FTextureSamplerDesc, Microsoft::WRL::ComPtr<ID3D11SamplerState>> SamplerStateMap;

	// 텍스트 인스턴싱 버퍼
	Microsoft::WRL::ComPtr<ID3D11Buffer> InstanceBuffer;
	UINT TextInstanceBufferSize = 0;
	FWString CachedBillboardText;
	const FFont* CachedBillboardFont = nullptr;
	TArray<FInstanceData> CachedBillboardGlyphs;
	TArray<FInstanceData> BillboardWorldInstances;

	// GPU 타이머
	struct FGPUTimerQuery
	{
		Microsoft::WRL::ComPtr<ID3D11Query> Disjoint;
		Microsoft::WRL::ComPtr<ID3D11Query> Start;
		Microsoft::WRL::ComPtr<ID3D11Query> End;
		// 이 프레임이 반영한 입력의 QPC 시각. 입력이 없었으면 0.
		int64 InputStartTick = 0;
		bool bPending = false;
	};

	FGPUTimerQuery GPUTimerQueries[GPUTimerFrameCount];
	uint32 GPUTimerFrameIndex = 0u;
	// 회수에 실패한 프레임에 0을 넣으면 평균이 눌리므로 직전 값을 들고 있는다.
	double LastGPUTimeMs = 0.0;

	// 중복 바인딩 방지용 마지막 상태
	const FMesh* LastMesh = nullptr;
	const FTexture* LastTexture = nullptr;
	bool bHasLastTexture = false;
	const FRenderPipeline* LastRenderPipeline = nullptr;

	// Draw/DrawSection이 드로우마다 통계 매크로를 부르지 않도록 여기에 모았다가
	// FlushDrawStats에서 한 번에 반영한다.
	uint32 PendingDrawCount = 0u;
	uint32 PendingPrimCount = 0u;

	// 프레임 리소스 링
	FFrameResource FrameResources[NumFrameResourceCount];
	uint32 CurrentFrameResourceIndex = 0;

	// 오클루전 오라클 (측정 도구)
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> OracleDepthState;
	Microsoft::WRL::ComPtr<ID3D11BlendState> OracleBlendState;
	TArray<Microsoft::WRL::ComPtr<ID3D11Query>> OracleQueries;
};
