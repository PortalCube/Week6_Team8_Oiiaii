#include "AActor.h"
#include "Runtime/Components/USpotLightComponent.h"

class ASpotlightActor : public AActor
{
	DECLARE_UCLASS(ASpotlightActor, AActor)
	GENERATED_BODY()

public:
	explicit ASpotlightActor();

	USpotLightComponent* GetSpotlightComponent() const;
};
