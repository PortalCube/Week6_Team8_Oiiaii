#pragma once

/*
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
	// 생명 주기 함수들
	////////////////////////////////////////////////////////////

	virtual void Initialize() override;
	virtual void Release() override;
	virtual void Register(ULevel& InScene);
	virtual void BeginPlay();
	virtual void Update(float DeltaTime) {}
	virtual void EndPlay();
	virtual void Unregister();

	bool HasBegunPlay() const { return bHasBegunPlay; }
	bool IsTickEnabled() const { return bTickEnabled; }

	////////////////////////////////////////////////////////////
	// 직렬화, 역직렬화
	////////////////////////////////////////////////////////////

	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;

protected:
	bool bHasBegunPlay = false;
	bool bTickEnabled = false;
	bool bInheritRotation = true;

};

*/
