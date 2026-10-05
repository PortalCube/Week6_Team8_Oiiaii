#include "AActor.h"
#include "Runtime/Components/USpotLightComponent.h"

class ASpotlightActor : public AActor
{
	DECLARE_UCLASS(ASpotlightActor, AActor)
	GENERATED_BODY()

public:
	virtual void Initialize() override;
	USpotLightComponent* GetSpotlightComponent() const;
};
