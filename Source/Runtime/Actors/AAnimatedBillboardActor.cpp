#include "AAnimatedBillboardActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/UAnimatedBillboardComp.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AAnimatedBillboardActor, AActor)
UCLASS_META(AAnimatedBillboardActor, DisplayName, "Animated Billboard Actor")

void AAnimatedBillboardActor::Initialize()
{
	Super::Initialize();
	bTickEnabled = true;

	// 루트 컴포넌트 생성 및 장착
	auto Component = CreateDefaultSubobject<UAnimatedBillboardComp>();
	if (Component)
	{
		SetRootComponent(Component);

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		UTexture* ExplosionTexture = Registry.Get<UTexture>("Texture/Explosion.json");

		// 폭발 스프라이트 텍스처 지정
		Component->SetTexture(ExplosionTexture);

		// 시트 분할 및 루프 재생 설정
		Component->SetSpriteSheet(6, 6, 20.0f, 36);
		Component->SetLooping(true);
		Component->Play();
	}
}
