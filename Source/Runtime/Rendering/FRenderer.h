#pragma once

#include "FMaterial.h"
#include "FMesh.h"
#include "FRenderPipeline.h"
#include "FRenderResourceLibrary.h"
#include "Runtime/Engine/Types/IntTypes.h"
#include "Runtime/Engine/Types/PointerTypes.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/CoreUObject/FStatsManager.h"
#include "Runtime/Material/FTextureSamplerDesc.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Rendering/FLineBatcher.h"
#include "ShaderConstants.h"
#include "Vertices.h"

#include <Windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <wrl/client.h>
#include <span>

class FTexture;
struct FTextureDesc;
class FCamera;
class UTextComponent;
struct FDrawCommand;
struct FPointLightConstants;

struct FFrameResource
{
	Microsoft::WRL::ComPtr<ID3D11Buffer> FrameConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> ViewConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> ObjectConstantBuffer;
};

// TODO: 임시 위치  헤더 파일로 분리
// Screen Pass에 쓰이는 Color와 Depth Texture
// 헤더 파일로 분리 헤더 파일로 분리 헤더 파일로 분리 헤더 파일로 분리 헤더 파일로 분리 헤더 파일로 분리 헤더 파일로 분리
struct FSceneTextures
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> SceneColorTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> SceneColorRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SceneColorSRV;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> SceneDepthTexture;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> SceneDepthDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SceneDepthSRV;

	void Reset()
	{
		SceneColorTexture.Reset();
		SceneColorRTV.Reset();
		SceneColorSRV.Reset();

		SceneDepthTexture.Reset();
		SceneDepthDSV.Reset();
		SceneDepthSRV.Reset();
	}
};


class FRenderer final
{
public:
	bool Initialize(HWND Window);
	void Shutdown();
	void BeginFrame();
	void BindSceneRenderTargets();
	void BindBackBufferRenderTargets();
	void SetViewportPixel(FVector2 LeftTopPixel, FVector2 RightBottomPixel);
	void ClearDepth();
	void SwapBuffer();
	void FlushDrawStats();
	void OnWindowSize(UINT Width, UINT Height);

	[[nodiscard]] TSharedPtr<FMesh> CreateMesh(const FMeshDesc& Desc);
	[[nodiscard]] TSharedPtr<FMesh> CreateDynamicMesh(const FMeshDesc& Desc); // 텍스트 렌더링용

	void GetDeviceAndContext_ImplDX11(ID3D11Device*& DeviceOut, ID3D11DeviceContext*& ContextOut);
	[[nodiscard]] ID3D11Device* GetDevice() const { return Device.Get(); }
	[[nodiscard]] ID3D11DeviceContext* GetContext() const { return Context.Get(); }

	[[nodiscard]] TSharedPtr<FRenderPipeline> CreateRenderPipeline(
		const FRenderPipelineDesc& Desc);
	[[nodiscard]] TSharedPtr<FTexture> CreateTexture(const wchar_t* path);
	TSharedPtr<FTexture> CreateSolidTexture(const FVector4& Color);

	// 파이프라인 조회
	[[nodiscard]] TSharedPtr<FRenderPipeline> GetPipeline(const FName& Id) const;

	FLineBatcher& GetLineBatcher() { return LineBatcher; }

	void UpdateLightConstants(const FLightConstants& Constants);
	void UploadPointLights(std::span<const FPointLightConstants> PointLights);
	void BindPointLights();
	void UpdateFrameConstants(const FFrameConstants& Constants);
	void UpdateViewConstants(const FViewConstants& Constants);

	// 텍스트 인스턴싱
	void AddTextInstanceArray(const FDrawCommand& Command);
	void DrawInstances(const FCamera& Camera, FRenderPipeline* OverridePipeline, bool bDisableShading);
	void DrawTextInstances(const FDrawCommand& Command);
	void ClearTextInstances();

	void Draw(const FDrawCommand& Command, FRenderPipeline* OverridePipeline, uint32 Slot = 2);

	void DrawPrimitiveBatch(std::span<const FDrawCommand> Commands, FRenderPipeline* OverridePipeline);

	bool UploadObjectConstants(std::span<const FDrawCommand> Commands);

	void BindObjectConstantRange(uint32 Slot, uint32 ByteOffset);
	void BindDrawResources(const FMesh& Mesh, const FMaterial& Material, FRenderPipeline* OverridePipeline);

	void DrawUploadedCommand(const FDrawCommand& Command, FRenderPipeline* OverridePipeline);

	void DrawScreenPass(ID3D11ShaderResourceView* SRVs[], ID3D11RenderTargetView* BackBuffer);
	void RenderSelectionOutline(FVector2 LeftTopPixel, FVector2 RightBottomPixel);
	void RenderSceneDepth();
	ID3D11RenderTargetView* GetBackBufferRTV() { return BackBufferRTV.Get(); }
	ID3D11DepthStencilView* GetSceneDepthDSV() { return SceneDepthDSV.Get(); }

	float GetWidth() const { return Viewport.Width; }
	float GetHeight() const { return Viewport.Height; }

	void ClearLastRenderState();

private:
	bool InitializeDeviceAndSwapChain(HWND Window);
	bool InitializeBackBuffer();
	bool InitializeSceneTextures();
	bool InitializeConstantBuffers();
	bool InitializeGPUTimerQueries();

	void ResetSceneTexture();

	// GPU 타임스탬프. 결과를 같은 프레임에 바로 읽으면 CPU가 GPU를 기다리게 되므로
	// 쿼리 세트를 돌려 쓰고 가장 오래된 것만 회수한다.
	void BeginGPUTimer();
	void EndGPUTimer();
	void ResolveGPUTimer();

	Microsoft::WRL::ComPtr<ID3D11RasterizerState>
	GetOrCreateRasterizerState(const FRasterizerDesc& Desc);
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>
	GetOrCreateDepthStencilState(const FDepthStencilDesc& Desc);
	Microsoft::WRL::ComPtr<ID3D11BlendState>
	GetOrCreateBlendState(const FBlendDesc& Desc);
	Microsoft::WRL::ComPtr<ID3D11SamplerState>
	GetOrCreateSamplerState(const FTextureSamplerDesc& Desc);

	FLineBatcher LineBatcher;
	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;
	D3D11_VIEWPORT Viewport{};

	// D3D11_1 Extension
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> Context1;

	// 모든 ConstantBuffer의 최대 크기
	static constexpr UINT ConstantBufferSize = 256u;

	// 상수 버퍼들
	/*Microsoft::WRL::ComPtr<ID3D11Buffer> FrameConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> ViewConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> ObjectConstantBuffer;*/
	Microsoft::WRL::ComPtr<ID3D11Buffer> LightConstantBuffer;

	static constexpr uint32 MaxPointLightCount = 64;

	Microsoft::WRL::ComPtr<ID3D11Buffer> PointLightBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> PointLightSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> PointLightCountBuffer;

	bool InitializePointLightBuffers();


	// 임시 상수버퍼
	Microsoft::WRL::ComPtr<ID3D11Buffer> ObjectConstantUploadBuffer;

	// Draw, ImGui 모두 다 포함하는 BackBuffer Texture
	Microsoft::WRL::ComPtr<ID3D11Texture2D> BackBufferTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> BackBufferRTV;

	// Screen Pass에서 쓰이는 텍스쳐
	Microsoft::WRL::ComPtr<ID3D11Texture2D> SceneColorTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> SceneColorRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SceneColorSRV;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> SceneDepthTexture; // 이전 DepthStencilBuffer
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> SceneDepthDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SceneDepthSRV;

	TMap<FRasterizerDesc, Microsoft::WRL::ComPtr<ID3D11RasterizerState>> RasterizerStateMap;
	TMap<FDepthStencilDesc, Microsoft::WRL::ComPtr<ID3D11DepthStencilState>> DepthStencilStateMap;
	TMap<FBlendDesc, Microsoft::WRL::ComPtr<ID3D11BlendState>> BlendStateMap;
	TMap<FTextureSamplerDesc, Microsoft::WRL::ComPtr<ID3D11SamplerState>> SamplerStateMap;

	// 텍스트 인스턴싱 버퍼
	Microsoft::WRL::ComPtr<ID3D11Buffer> InstanceBuffer;
	UINT TextInstanceBufferSize = 0;

	struct FGPUTimerQuery
	{
		Microsoft::WRL::ComPtr<ID3D11Query> Disjoint;
		Microsoft::WRL::ComPtr<ID3D11Query> Start;
		Microsoft::WRL::ComPtr<ID3D11Query> End;
		// 이 프레임이 반영한 입력의 QPC 시각. 입력이 없었으면 0.
		int64 InputStartTick = 0;
		bool bPending = false;
	};

	static constexpr uint32 GPUTimerFrameCount = 3u;
	FGPUTimerQuery GPUTimerQueries[GPUTimerFrameCount];
	uint32 GPUTimerFrameIndex = 0u;

	// 회수에 실패한 프레임에 0을 넣으면 평균이 눌리므로 직전 값을 들고 있는다.
	double LastGPUTimeMs = 0.0;

	const FMesh* LastMesh = nullptr;
	const FTexture* LastTexture = nullptr;
	bool bHasLastTexture = false;
	const FRenderPipeline* LastRenderPipeline = nullptr;

	// Draw/DrawSection이 드로우마다 통계 매크로를 부르지 않도록 여기에 모았다가
	// FlushDrawStats에서 한 번에 반영한다.
	uint32 PendingDrawCount = 0u;
	uint32 PendingPrimCount = 0u;

	static constexpr uint32 NumFrameResourceCount = 3;
	FFrameResource FrameResources[NumFrameResourceCount];
	uint32 CurrentFrameResourceIndex = 0;

	FFrameResource* GetCurrentFrameResource() { return &FrameResources[CurrentFrameResourceIndex]; }
	FFrameResource* GetNextFrameResource() { return &FrameResources[(CurrentFrameResourceIndex + 1) % NumFrameResourceCount]; }

public:
	template <typename TConstants>
	void FlushLineBatch( const TConstants& Constants, const FName& PipelineId = FName("Simple_Line"))
	{
		UpdateBuffer(Constants, 2);
		LineBatcher.Flush(*Context.Get(), GetPipeline(PipelineId));
	}

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

	// Constant Buffer를 갱신한다.
	// 크기가 맞는지는 컴파일 타임에 검사한다.
	/*  template <typename TConstants>
	  void UpdateBuffer(const TConstants &Constants, uint32 Slot) {
	    static_assert(sizeof(TConstants) <= ConstantBufferSize);
	    static_assert(sizeof(TConstants) % 16 == 0);

	    // 언리얼 Clip -> D3D Clip 좌표 변환.
	    // MVP, VP를 가진 상수 타입에만 적용한다(없는 타입은 그대로 통과).
	    TConstants ShaderConstants = Constants;
	    if constexpr (requires { ShaderConstants.MVP; }) {
	        ShaderConstants.MVP = ShaderConstants.MVP.ToD3DMatrix();
	    }

	    D3D11_MAPPED_SUBRESOURCE Mapped{};
	    if (FAILED(Context->Map(ObjectConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD,
	                            0, &Mapped))) {
	      return;
	    }
	    std::memcpy(Mapped.pData, &ShaderConstants, sizeof(TConstants));
	    Context->Unmap(ObjectConstantBuffer.Get(), 0);

	    Context->VSSetConstantBuffers(Slot, 1u, ObjectConstantBuffer.GetAddressOf());
	    Context->PSSetConstantBuffers(Slot, 1u, ObjectConstantBuffer.GetAddressOf());
	  }*/

	template <typename TConstants>
	void UpdateBuffer(const TConstants& Constants, uint32 Slot)
	{
		static_assert(sizeof(TConstants) <= ConstantBufferSize);
		static_assert(sizeof(TConstants) % 16 == 0);

		// 언리얼 Clip -> D3D Clip 좌표 변환.
		// MVP, VP를 가진 상수 타입에만 적용한다(없는 타입은 그대로 통과).
		TConstants ShaderConstants = Constants;
		if constexpr (requires { ShaderConstants.MVP; })
		{
			ShaderConstants.MVP = ShaderConstants.MVP.ToD3DMatrix();
		}

		D3D11_MAPPED_SUBRESOURCE Mapped{};
		if (FAILED(Context->Map(GetCurrentFrameResource()->ObjectConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped)))
		{
			return;
		}
		std::memcpy(Mapped.pData, &ShaderConstants, sizeof(TConstants));
		Context->Unmap(GetCurrentFrameResource()->ObjectConstantBuffer.Get(), 0);

		Context->VSSetConstantBuffers(Slot, 1u, GetCurrentFrameResource()->ObjectConstantBuffer.GetAddressOf());
		Context->PSSetConstantBuffers(Slot, 1u, GetCurrentFrameResource()->ObjectConstantBuffer.GetAddressOf());
	}

public:
	// 현재 깊이 버퍼 기준으로 각 명령이 실제로 보이는 픽셀 수를 GPU에 묻는다.
	// GPU가 끝날 때까지 기다리므로 느리다. 디버깅에서 쓰는 한 프레임 측정 전용
	void QueryVisibility(const TArray<const FDrawCommand*>& Commands, TArray<uint64>& OutSamples);

private:
	// 오클루전 오라클 (측정 도구)
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> OracleDepthState;
	Microsoft::WRL::ComPtr<ID3D11BlendState> OracleBlendState;
	TArray<Microsoft::WRL::ComPtr<ID3D11Query>> OracleQueries;
};
