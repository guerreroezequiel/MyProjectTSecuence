#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "AnimLocomotionFragment.generated.h"

UENUM(BlueprintType)
enum class EAnimLocomotionState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Walk UMETA(DisplayName = "Walk")
};

USTRUCT(BlueprintType)
struct MYPROJECTTSECUENCE_API FAnimLocomotionFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Anim")
	EAnimLocomotionState CurrentState = EAnimLocomotionState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Anim")
	float LastPlayRate = 1.0f;
};
