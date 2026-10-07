#include "UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Serialization/FArchive.h"
#include "Runtime/Core/Globals.h"
#include <numbers>

IMPLEMENT_UCLASS(UStaticMeshComponent, UMeshComponent)
UCLASS_META(UStaticMeshComponent, DisplayName, "Static Mesh Component")

float UStaticMeshComponent::ComputeScreenSize(const FCamera& Camera) const
{
	return std::sqrt(ComputeScreenSizeSquared(Camera));
}

float UStaticMeshComponent::ComputeScreenSizeSquared(const FCamera& Camera) const
{
	return ComputeScreenSizeSquared(GetWorldBounds(), Camera);
}

float UStaticMeshComponent::ComputeScreenSizeSquared(const FAxisAlignedBoundingBox& Bounds, const FCamera& Camera)
{
	return ComputeScreenSizeSquared(Bounds, MakeLODView(Camera));
}

UStaticMeshComponent::FLODView UStaticMeshComponent::MakeLODView(const FCamera& Camera)
{
	FLODView View;
	const FVector& CameraPosition = Camera.GetPosition();
	View.CamX = CameraPosition.X;
	View.CamY = CameraPosition.Y;
	View.CamZ = CameraPosition.Z;

	// 배율(1/tan(FOV/2))은 투영이 바뀔 때 FCameraProjection에서 한 번만 계산해 둔다.
	const FCameraProjection& Projection = Camera.GetProjection();
	View.bOrthographic = Projection.GetProjectionType() == EProjectionType::Orthographic;
	const float Multiple = Projection.GetScreenSizeMultiple();
	View.MultipleSq = Multiple * Multiple;
	const float Height = std::max(Projection.GetOrthographicHeight(), 1e-4f);
	View.OrthoHeightSq = Height * Height;
	return View;
}

float UStaticMeshComponent::ComputeScreenSizeSquared(const FAxisAlignedBoundingBox& Bounds, const FLODView& View)
{
	// 매 프레임 오브젝트마다 도는 코드라, 함수 호출(IsValid, std::max, FVector 연산자) 없이 float로만 계산한다.
	// 월드 바운드의 Center/Extent는 바운드가 갱신될 때 이미 계산되어 있다.
	if (Bounds.Min.X > Bounds.Max.X || Bounds.Min.Y > Bounds.Max.Y || Bounds.Min.Z > Bounds.Max.Z)
	{
		return 1.0f;
	}

	// 바운딩 박스를 감싸는 구로 근사한다. 반지름 = Extent의 길이.
	const float EX = Bounds.Extent.X, EY = Bounds.Extent.Y, EZ = Bounds.Extent.Z;
	const float RadiusSq = EX * EX + EY * EY + EZ * EZ;

	if (View.bOrthographic)
	{
		return 4.0f * RadiusSq / View.OrthoHeightSq;
	}

	// 언리얼의 ComputeBoundsScreenSize와 같은 방식. 구의 지름이 화면 높이의 몇 배인지의 제곱을 반환한다.
	const float DX = Bounds.Center.X - View.CamX;
	const float DY = Bounds.Center.Y - View.CamY;
	const float DZ = Bounds.Center.Z - View.CamZ;
	float DistanceSq = DX * DX + DY * DY + DZ * DZ;
	DistanceSq = DistanceSq > 1e-8f ? DistanceSq : 1e-8f;
	return View.MultipleSq * RadiusSq / DistanceSq;
}

uint32 UStaticMeshComponent::SelectLOD(const FCamera& Camera) const
{
	return SelectLOD(RenderData.Mesh, GetWorldBounds(), Camera);
}

uint32 UStaticMeshComponent::SelectLOD(const UStaticMesh* Mesh, const FAxisAlignedBoundingBox& WorldBounds, const FCamera& Camera)
{
	return SelectLOD(Mesh, WorldBounds, MakeLODView(Camera));
}

uint32 UStaticMeshComponent::SelectLOD(const UStaticMesh* Mesh, const FAxisAlignedBoundingBox& WorldBounds, const FLODView& View)
{
	if (!Globals::bEnableLOD)
	{
		return 0;
	}
	if (!Mesh)
	{
		return 0;
	}

	const uint32 LODCount = Mesh->GetLODCount();
	if (LODCount <= 1)
	{
		return 0;
	}

	if (Globals::ForcedLOD >= 0)
	{
		const uint32 Forced = static_cast<uint32>(Globals::ForcedLOD);
		return Forced < LODCount - 1 ? Forced : LODCount - 1;
	}

	return Mesh->SelectLODSquared(ComputeScreenSizeSquared(WorldBounds, View));
}

FAxisAlignedBoundingBox UStaticMeshComponent::GetLocalBounds() const
{
	const FMesh* Mesh = RenderData.Mesh ? RenderData.Mesh->Get() : nullptr;
	return Mesh ? Mesh->GetLocalBounds() : FAxisAlignedBoundingBox{};
}

void UStaticMeshComponent::SetMesh(UStaticMesh* Mesh)
{
	UPrimitiveComponent::SetMesh(Mesh);

	const TArray<FMeshSection>& Sections = Mesh->Get()->GetSections();
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	UMaterial* FallbackMaterial = Registry.Get<UMaterial>("Material/Simple.json");

	RenderData.Materials.clear();
	for (size_t i = 0; i < Sections.size(); i++)
	{
		UMaterial* Mat = Registry.Get<UMaterial>(FName(Sections[i].SectionName));
		SetMaterial(Mat ? Mat : FallbackMaterial, static_cast<int32>(i));
	}

	// 메시에서 유효한 Material 정보를 하나도 찾지 못하면 기본 Material을 사용한다.
	if (RenderData.Materials.empty())
	{
		SetMaterial(FallbackMaterial, 0);
	}
}

const UMaterial* UStaticMeshComponent::GetMaterial(int Index) const
{
	const FMaterialInstance* Instance = GetMaterialInstance(Index);
	return Instance ? Instance->Material : nullptr;
}

const FMaterialInstance* UStaticMeshComponent::GetMaterialInstance(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size())
	{
		return nullptr;
	}
	return &RenderData.Materials[static_cast<size_t>(Index)];
}

int32 UStaticMeshComponent::GetMaterialSlotLength() const
{
	if (RenderData.Mesh && RenderData.Mesh->Get())
	{
		// 잘못된 메시가 섹션 없이 들어와도 Material을 지정할 슬롯은 하나 제공한다.
		return std::max(1, static_cast<int32>(RenderData.Mesh->Get()->GetSectionCount()));
	}

	return static_cast<int32>(RenderData.Materials.size());
}

void UStaticMeshComponent::SetMaterialInstance(const FMaterialInstance& Instance, int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size())
	{
		return;
	}
	RenderData.Materials[static_cast<size_t>(Index)] = Instance;
	UpdateMaterialCache();
	UpdateSortKey();
}

void UStaticMeshComponent::SetPipeline(UPipeline* Pipeline, int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size())
	{
		return;
	}
	FMaterialInstance& Instance = RenderData.Materials[static_cast<size_t>(Index)];

	Instance.Pipeline = Pipeline;
	UpdateMaterialCache();
	UpdateSortKey();
}

void UStaticMeshComponent::SetTexture(UTexture* Texture, int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= RenderData.Materials.size())
	{
		return;
	}
	FMaterialInstance& Instance = RenderData.Materials[static_cast<size_t>(Index)];

	Instance.Texture = Texture;
	UpdateMaterialCache();
	UpdateSortKey();
}

void UStaticMeshComponent::ClearMaterial()
{
	RenderData.Materials.clear();
	UpdateMaterialCache();
	UpdateSortKey();
}

EEngineShowFlags UStaticMeshComponent::GetShowFlag() const
{
	return EEngineShowFlags::SF_Primitives;
}

void UStaticMeshComponent::Serialize(FArchive& Archive)
{
	Super::Serialize(Archive);

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	// 메시 가져오기
	FString MeshAssetID = "";
	if (RenderData.Mesh)
	{
		MeshAssetID = RenderData.Mesh->GetIDString();
	}
	Archive.Field("MeshAsset", MeshAssetID);

	if (Archive.IsReading() && !MeshAssetID.empty())
	{
		UStaticMesh* Mesh = Registry.Get<UStaticMesh>(MeshAssetID);
		if (Mesh)
		{
			SetMesh(Mesh);
		}
	}

	// Material 가져오기
	int32 Count = Archive.BeginArray("Materials");
	{
		if (Archive.IsWriting())
		{
			Count = static_cast<int32>(RenderData.Materials.size());
		}

		for (int32 Index = 0; Index < Count; ++Index)
		{
			UMaterial* InitialMaterial = RenderData.Materials[Index].Material;

			FString MaterialID = InitialMaterial->GetIDString();

			Archive.BeginSection("");
			{
				Archive.Field("MaterialAsset", MaterialID);

				UMaterial* Material = nullptr;

				if (Archive.IsReading())
				{
					Material = Registry.Get<UMaterial>(MaterialID);
				}
				else
				{
					Material = InitialMaterial;
				}

				FMaterialInstance Instance = Archive.IsWriting() ? RenderData.Materials[Index] : FMaterialInstance{ Material };

				FString PipelineID = Instance.Pipeline ? Instance.Pipeline->GetIDString() : "";
				FString TextureID = Instance.Texture ? Instance.Texture->GetIDString() : "";

				Archive.Field("OverridePipelineAsset", PipelineID);
				Archive.Field("OverrideTextureAsset", TextureID);

				if (Archive.IsReading())
				{
					if (UPipeline* Pipeline = Registry.Get<UPipeline>(PipelineID))
					{
						Instance.Pipeline = Pipeline;
					}

					if (UTexture* Texture = Registry.Get<UTexture>(TextureID))
					{
						Instance.Texture = Texture;
					}
				}

				Archive.Field("Albedo", Instance.Albedo);
				Archive.Field("Diffuse", Instance.Diffuse);
				Archive.Field("Specular", Instance.Specular);
				Archive.Field("DisableShading", Instance.bDisableShading);
				Archive.Field("Color", Instance.Color);
				Archive.Field("UVOffset", Instance.UVOffset);
				Archive.Field("UVScale", Instance.UVScale);

				if (Archive.IsReading())
				{
					SetMaterialInstance(Instance, Index);
				}
			}
			Archive.EndSection();
		}
	}
	Archive.EndArray();
}
