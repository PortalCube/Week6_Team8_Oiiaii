#pragma once

#include "Runtime/Geometry/FTransform.h"
#include "ThirdParty/Json/json.hpp"
#include "Runtime/CoreUObject/UObject.h"

class ULevel;
class AActor;
class FArchive;

class UActorComponent : public UObject
{
	GENERATED_BODY()
	DECLARE_UCLASS(UActorComponent, UObject)
	friend class AActor;

public:

	////////////////////////////////////////////////////////////
	// Lifecycle
	////////////////////////////////////////////////////////////

	virtual void Initialize() override;
	virtual void Release() override;

	virtual void OnRegister();
	virtual void OnUnregister();

	virtual void InitializeComponent() {}
	virtual void UninitializeComponent() {}

	virtual void RegisterComponent();
	virtual void UnregisterComponent();

	virtual void BeginPlay();
	virtual void TickComponent(float DeltaTime) {}
	virtual void EndPlay();



	////////////////////////////////////////////////////////////
	// Check Lifecycle Status
	////////////////////////////////////////////////////////////

	bool bHasBegunPlay = false;
	bool bTickEnabled = false;
	bool bRegistered = false;

	bool HasBegunPlay() const { return bHasBegunPlay; }
	bool IsTickEnabled() const { return bTickEnabled; }

	virtual void MarkAsEditorOnlySubobject() override;


	////////////////////////////////////////////////////////////
	// Get Owner
	////////////////////////////////////////////////////////////

	AActor* GetOwner() const;
	ULevel* GetComponentLevel() const;
	UWorld* GetWorld() const;

	virtual bool IsEditorOnly() const override;

	////////////////////////////////////////////////////////////
	// 직렬화, 역직렬화
	////////////////////////////////////////////////////////////

	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;

protected:

	UWorld* World;

	bool bIsEditorOnly = false;
	bool bIsVisualizationComponent = false;

};
