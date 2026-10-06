#include "pch.h"
#include "ACatActor.h"

#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(ACatActor, AActor)
UCLASS_META(ACatActor, DisplayName, "Cat Actor")

void ACatActor::Initialize()
{
	Super::Initialize();
	bTickEnabled = true;

	auto Component = CreateDefaultSubobject<UStaticMeshComponent>();
	if (Component)
	{
		SetRootComponent(Component);
		CatComponent = Component;

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		Component->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/oiia/oiia.json"));
	}
}

void ACatActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ElapsedTime += DeltaTime;

	if (ElapsedTime >= SpinRate)
	{
		ElapsedTime = 0.0f;

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		if (bIsSpin)
		{
			CatComponent->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/oiia/oiia.json"));
			bIsSpin = false;
		}
		else
		{
			CatComponent->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/oiia/Spin.json"));
			bIsSpin = true;
		}
	}

	if (bIsSpin)
	{
		FTransform CurrentTransform = GetTransform();
		FQuaternion Rotation = CurrentTransform.GetRotation();
		Rotation.RotateLocalAxisAngle(FVector(0.0f, 0.0f, 1.0f), SpinSpeed * DeltaTime);
		CurrentTransform.SetRotation(Rotation);
		SetTransform(CurrentTransform);
	}
}
