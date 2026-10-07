#pragma once

#include "Runtime/CoreUObject/UClass.h"

#include "Runtime/Actors/AActor.h"
#include "Runtime/Actors/AAppleNormalActor.h"
#include "Runtime/Actors/AAppleBittenActor.h"
#include "Runtime/Actors/ACubeActor.h"
#include "Runtime/Actors/ASphereActor.h"
#include "Runtime/Actors/ACylinderActor.h"
#include "Runtime/Actors/ABillboardActor.h"
#include "Runtime/Actors/AAnimatedBillboardActor.h"
#include "Runtime/Actors/ASpotlightActor.h"
#include "Runtime/Actors/ATextRenderActor.h"
#include "Runtime/Actors/AFireBallActor.h"
#include "Runtime/Actors/ACatActor.h"
#include "Runtime/Actors/AHeightFogActor.h"

#include "Runtime/Components/UBillboardComponent.h"
#include "Runtime/Components/UTextComponent.h"
#include "Runtime/Components/UFireBallComponent.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Components/UProjectileMovementComponent.h"
#include "Runtime/Components/URotationMovementComponent.h"

namespace EditorConstant
{

	// 에디터에서 스폰 가능한 액터들을 정의
	inline UClass* const SpawnableActors[]{
		AActor::StaticClass(),
		ACatActor::StaticClass(),
		AAppleNormalActor::StaticClass(),
		AAppleBittenActor::StaticClass(),
		ACubeActor::StaticClass(),
		ASphereActor::StaticClass(),
		ACylinderActor::StaticClass(),
		ABillboardActor::StaticClass(),
		AAnimatedBillboardActor::StaticClass(),
		ASpotlightActor::StaticClass(),
		ATextRenderActor::StaticClass(),
		AFireBallActor::StaticClass(),
		AHeightFogActor::StaticClass(),
	};
	
	// 에디터에서 스폰 가능한 컴포넌트들을 정의
	inline UClass* const SpawnableComponents[]{
		UBillboardComponent::StaticClass(),
		UTextComponent::StaticClass(),
		UFireBallComponent::StaticClass(),
		UStaticMeshComponent::StaticClass(),
		UProjectileMovementComponent::StaticClass(),
		URotationMovementComponent::StaticClass(),
	};

} // namespace EditorConstant
