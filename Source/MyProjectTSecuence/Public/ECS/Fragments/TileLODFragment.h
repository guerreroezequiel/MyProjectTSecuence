#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "TileLODFragment.generated.h"

UENUM(BlueprintType)
enum class ETileLOD : uint8
{
	Hot   UMETA(DisplayName="HOT"),
	Warm  UMETA(DisplayName="WARM"),
	Cold  UMETA(DisplayName="COLD")
};

USTRUCT(BlueprintType)
struct MYPROJECTTSECUENCE_API FTileLODFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LOD")
	ETileLOD LOD = ETileLOD::Cold;
};
