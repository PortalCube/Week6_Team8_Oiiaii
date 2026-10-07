#include "Runtime/CoreUObject/UClass.h"
#include "USceneComponent.h"
#include "ThirdParty/Json/json.hpp"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "UPrimitiveComponent.h"
#include "Runtime/Serialization/FArchive.h"
#include "Runtime/Engine/ULevel.h"
#include <algorithm>
#include <cmath>

IMPLEMENT_UCLASS(USceneComponent, UActorComponent)
UCLASS_META(USceneComponent, DisplayName, "Scene Component")

void USceneComponent::SetupAttachment(USceneComponent* InParent)
{
	if (InParent == nullptr)
	{
		return;
	}

	// 순환 참조 찾기
	USceneComponent* Node = InParent;
	while (Node != nullptr)
	{
		// 순환 참조 return
		if (Node == this)
		{
			return;
		}

		Node = Node->AttachParent;
	}

	AttachParent = InParent;
	InParent->Children.push_back(this);

	bGlobalDirty = true;
}

bool USceneComponent::AttachToComponent(USceneComponent* InParent)
{
	if (AttachParent)
	{
		DetachFromComponent();
	}

	SetupAttachment(InParent);
	return true;
}

void USceneComponent::DetachFromComponent()
{
	if (!AttachParent)
	{
		return;
	}

	TArray<USceneComponent*>& Children = AttachParent->Children;

	for (auto It = Children.begin(); It < Children.end(); ++It)
	{
		if (*It == this)
		{
			Children.erase(It);
			break;
		}
	}

	AttachParent = nullptr;
	bGlobalDirty = true;
}

void USceneComponent::Serialize(FArchive& Archive)
{
	Super::Serialize(Archive);

	FVector Location = RelativeTransform.GetLocation();
	FVector Rotation = RelativeTransform.GetRotation().GetEulerXYZ();
	FVector Scale = RelativeTransform.GetScale3D();

	Archive.Field("Location", Location);
	Archive.Field("Rotation", Rotation);
	Archive.Field("Scale", Scale);

	USceneComponent* PreviousParent = AttachParent;
	Archive.Reference("AttachParent", AttachParent);

	Archive.Field("InheritRotation", bInheritRotation);

	if (Archive.IsReading())
	{
		// 부모가 변경되었다면, 원래 부모의 자식 배열에서 제거
		if (PreviousParent && PreviousParent != AttachParent)
		{
			std::erase(PreviousParent->Children, this);
		}

		// 새로운 부모가 생겼다면 Children 배열에 자신을 넣기
		if (AttachParent)
		{
			TArray<USceneComponent*>& ParentChildren = AttachParent->Children;

			auto It = std::find(ParentChildren.begin(), ParentChildren.end(), this);

			if (It == ParentChildren.end())
			{
				ParentChildren.push_back(this);
			}
		}

		constexpr float RadToDeg = 180.0f / std::numbers::pi_v<float>;
		RelativeTransform.SetLocation(Location);
		RelativeTransform.SetRotation(FQuaternion::FromEulerXYZDeg(Rotation * RadToDeg));
		RelativeTransform.SetScale3D(Scale);
		MarkActorTransformDirty();
	}
}

void USceneComponent::SetRelativeTransform(const FTransform& RelativeTransform)
{
	if (this->RelativeTransform == RelativeTransform)
	{
		return;
	}
	this->RelativeTransform = RelativeTransform;
	MarkActorTransformDirty();
}

bool USceneComponent::SetGlobalTransform(const FTransform& GlobalTransform)
{
	const USceneComponent* Parent = GetTransformParent();

	if (!Parent)
	{
		SetRelativeTransform(GlobalTransform);
		return true;
	}

	const FTransform& ParentTransform = Parent->GetGlobalTransform();
	FTransform NewRelativeTransform = GlobalTransform;

	if (AttachParent || bInheritRotation)
	{
		const FVector& ParentScale = ParentTransform.GetScale3D();
		constexpr float MinScale = 1e-6f;
		if (std::abs(ParentScale.X) < MinScale ||
			std::abs(ParentScale.Y) < MinScale ||
			std::abs(ParentScale.Z) < MinScale)
		{
			return false;
		}

		// GetGlobalTransform의 부모 * 자식 합성을 역순으로 풀어낸다
		const FQuaternion InverseParentRotation = ParentTransform.GetRotation().Normalized().Conjugate();
		const FVector LocalLocation = InverseParentRotation.RotateVector(
			GlobalTransform.GetLocation() - ParentTransform.GetLocation());
		const FVector& GlobalScale = GlobalTransform.GetScale3D();

		NewRelativeTransform.SetLocation(FVector{
			LocalLocation.X / ParentScale.X,
			LocalLocation.Y / ParentScale.Y,
			LocalLocation.Z / ParentScale.Z });
		NewRelativeTransform.SetRotation((InverseParentRotation * GlobalTransform.GetRotation()).Normalized());
		NewRelativeTransform.SetScale3D(FVector{
			GlobalScale.X / ParentScale.X,
			GlobalScale.Y / ParentScale.Y,
			GlobalScale.Z / ParentScale.Z });
	}
	else
	{
		// 명시적인 부착 없이 루트 회전을 무시하는 경우에는 위치만 되돌린다.
		NewRelativeTransform.SetLocation(GlobalTransform.GetLocation() - ParentTransform.GetLocation());
	}

	SetRelativeTransform(NewRelativeTransform);
	return true;
}

USceneComponent* USceneComponent::GetTransformParent() const
{
	if (AttachParent)
	{
		return AttachParent;
	}

	USceneComponent* Root = GetOwner()->GetRootComponent();
	return Root == this ? nullptr : Root;
}

const FTransform& USceneComponent::GetGlobalTransform() const // 나중에 부모 rootcomponent world좌표 써야됨
{
	const USceneComponent* Parent = GetTransformParent();
	uint32 ParentVersion = 0;
	if (Parent)
	{
		Parent->GetGlobalTransform(); // 부모 캐시를 먼저 최신화
		ParentVersion = Parent->GlobalVersion;
	}

	if (!bGlobalDirty && CachedParent == Parent && CachedParentVersion == ParentVersion)
	{
		return CachedGlobal;
	}

	if (!Parent)
	{
		CachedGlobal = RelativeTransform;
	}
	else if (AttachParent || bInheritRotation)
	{
		CachedGlobal = Parent->CachedGlobal * RelativeTransform;
	}
	else
	{
		// 부모 회전 무시 - 위치와 스케일만 상속
		FTransform Result;
		Result.SetScale3D(RelativeTransform.GetScale3D());
		Result.SetRotation(RelativeTransform.GetRotation());                                      // 자신의 회전만 사용
		Result.SetLocation(Parent->CachedGlobal.GetLocation() + RelativeTransform.GetLocation()); // 월드 축 기준 오프셋
		CachedGlobal = Result;
	}

	CachedGlobal.GetMatrix(); // 행렬도 이 시점에 한 번만 계산해 둔다
	CachedParent = Parent;
	CachedParentVersion = ParentVersion;
	bGlobalDirty = false;
	++GlobalVersion;
	return CachedGlobal;
}

const FMatrix* USceneComponent::GetGlobalInverseMatrix() const
{
	const FTransform& Global = GetGlobalTransform();
	if (CachedInverseVersion != GlobalVersion)
	{
		// 실패하면 Inverse는 CachedGlobalInverse를 건드리지 않으므로 성공 여부를 따로 기록한다
		bCachedInverseValid = Global.GetMatrix().Inverse(CachedGlobalInverse);
		CachedInverseVersion = GlobalVersion;
	}
	return bCachedInverseValid ? &CachedGlobalInverse : nullptr;
}

void USceneComponent::MarkActorTransformDirty()
{
	bGlobalDirty = true;
	OnTransformChanged();

	GetOwner()->MarkComponentsTransformDirty();
}

void USceneComponent::SetRelativeLocation(const FVector& RelativeLocation)
{
	FTransform NewTransform = GetRelativeTransform();
	NewTransform.SetLocation(RelativeLocation);
	SetRelativeTransform(NewTransform);
}

void USceneComponent::SetRelativeRotation(const FVector& RelativeRotationEulerAngle)
{
	FTransform NewTransform = GetRelativeTransform();
	NewTransform.SetRotation(FQuaternion::FromEulerXYZDeg(RelativeRotationEulerAngle));
	SetRelativeTransform(NewTransform);
}

void USceneComponent::SetRelativeRotation(const FQuaternion& RelativeRotation)
{
	FTransform NewTransform = GetRelativeTransform();
	NewTransform.SetRotation(RelativeRotation);
	SetRelativeTransform(NewTransform);
}

void USceneComponent::SetRelativeScale(const FVector& RelativeScale)
{
	FTransform NewTransform = GetRelativeTransform();
	NewTransform.SetScale3D(RelativeScale);
	SetRelativeTransform(NewTransform);
}

const FVector& USceneComponent::GetRelativeLocation() const
{
	return GetRelativeTransform().GetLocation();
}

const FQuaternion& USceneComponent::GetRelativeRotation() const
{
	return GetRelativeTransform().GetRotation();
}

const FVector& USceneComponent::GetRelativeScale() const
{
	return GetRelativeTransform().GetScale3D();
}
