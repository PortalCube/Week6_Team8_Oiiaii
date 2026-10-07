#pragma once

#include "FFont.h"
#include "FInstanceBatchKey.h"
#include "FMaterial.h"
#include "FMesh.h"
#include "FRenderPipeline.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/FName.h"
#include "Runtime/Engine/Types/PointerTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include "Vertices.h"

class FRenderer;
class FTexture;

class FRenderResourceLibrary final
{
public:
	// 전역 싱글톤 접근자
	static FRenderResourceLibrary& Get();

	bool Initialize(FRenderer& Renderer);

	// 파이프라인 보관 맵
	TMap<FName, TSharedPtr<FRenderPipeline>> AllPipelineMap;
	// 메쉬 보관 맵
	TMap<FName, TSharedPtr<FMesh>> AllMeshMap;
	// 머티리얼 보관 맵 (FName 기반)
	TMap<FName, TSharedPtr<FMaterial>> AllMaterialMap;
	// 텍스쳐 보관 맵 (FName 기반)
	TMap<FName, TSharedPtr<FTexture>> AllTextureMap;
	// 폰트 보관 맵
	TMap<FName, TSharedPtr<FFont>> AllFontMap;
	// 인스턴싱 배치 배열 맵
	TMap<FInstanceBatchKey, TArray<FInstanceData>> AllInstancingArrayMap;

	// 파이프라인
	TSharedPtr<FRenderPipeline> GetPipeline(const FName& Id) const
	{
		auto it = AllPipelineMap.find(Id);
		if (it != AllPipelineMap.end())
		{
			return it->second;
		}
		return nullptr;
	}
	void RegisterPipeline(const FName& Id, TSharedPtr<FRenderPipeline> Pipeline) { AllPipelineMap[Id] = std::move(Pipeline); }


	// 인스턴싱 배열
	TArray<FInstanceData>& GetInstancingArray(const FMesh* Mesh, const FMaterial* Material) { return AllInstancingArrayMap[{ Mesh, Material }]; }

	// 메쉬 
	TSharedPtr<FMesh> RegisterMesh(const FName& ID, TSharedPtr<FMesh> inMesh)
	{
		inMesh->MeshId = ID;
		AllMeshMap[ID] = inMesh;
		return inMesh;
	}
	TSharedPtr<FMesh> GetMesh(const FName& ID) const
	{
		auto it = AllMeshMap.find(ID);
		if (it != AllMeshMap.end())
		{
			return it->second;
		}
		return nullptr;
	}

	// 머티리얼 
	TSharedPtr<FMaterial> RegisterMaterial(const FName& Id, TSharedPtr<FMaterial> inMaterial);
	TSharedPtr<FMaterial> GetMaterial(const FName& Id) const
	{
		auto it = AllMaterialMap.find(Id);
		if (it != AllMaterialMap.end())
		{
			return it->second;
		}
		return nullptr;
	}

	// 텍스쳐 
	void RegisterTexture(const FName& name, TSharedPtr<FTexture> texture) { AllTextureMap[name] = texture; }
	TSharedPtr<FTexture> GetTexture(const FName& name) const
	{
		auto it = AllTextureMap.find(name);
		if (it != AllTextureMap.end())
		{
			return it->second;
		}
		return nullptr;
	}

	// 맵 해제
	void DestroyAllMeshes() { AllMeshMap.clear(); }
	void DestroyAllMaterials() { AllMaterialMap.clear(); }
	void DestroyAllPipelines() { AllPipelineMap.clear(); }
	void DestroyAllInstancingArray() { AllInstancingArrayMap.clear(); }

	// 맵 조회
	const TMap<FName, TSharedPtr<FMaterial>>& GetAllMaterials() const { return AllMaterialMap; }
	const TMap<FName, TSharedPtr<FRenderPipeline>>& GetAllPipelines() const { return AllPipelineMap; }
	const TMap<FName, TSharedPtr<FTexture>>& GetAllTextures() const { return AllTextureMap; }

	// 렌더러 참조 조회
	FRenderer* GetRenderer() const { return RendererRef; }

	// 정점 배열 메쉬 캐싱 생성
	TSharedPtr<FMesh> GetOrCreateMesh(const FName& ID, const TArray<FVertexData>& vertices);

	TSharedPtr<FFont> GetFont(const FName& InName) const
	{
		auto it = AllFontMap.find(InName);
		if (it != AllFontMap.end())
		{
			return it->second;
		}
		return nullptr;
	}

private:
	bool InitializePipelines(FRenderer& Renderer);
	bool CreateWireframePipeline(FRenderer& Renderer);
	bool CreateSelectionStencilPipeline(FRenderer& Renderer);
	bool CreateSelectionOutlinePipeline(FRenderer& Renderer);
	bool CreateCompositePipeline(FRenderer& Renderer);
	bool CreateFogPipeline(FRenderer& Renderer);
	bool CreateSceneDepthPipeline(FRenderer& Renderer);
	bool CreateCustomPipline(
	    FRenderer& Renderer,
	    FWString VertexShaderPath,
	    FWString PixelShaderPath,
	    FString PipelineName,
	    bool StencilEnable = false,
	    D3D11_STENCIL_OP StencilPassOp = D3D11_STENCIL_OP_KEEP,
	    D3D11_COMPARISON_FUNC StencilFunc = D3D11_COMPARISON_ALWAYS,
	    UINT RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	    D3D11_FILTER SamplerFilter = D3D11_FILTER_MIN_MAG_MIP_POINT,
	    D3D11_TEXTURE_ADDRESS_MODE AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
	    D3D11_TEXTURE_ADDRESS_MODE AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
	    D3D11_TEXTURE_ADDRESS_MODE AddressW = D3D11_TEXTURE_ADDRESS_CLAMP);
	bool CreateInstancingArrayMap();
	FRenderer* RendererRef = nullptr;
};
