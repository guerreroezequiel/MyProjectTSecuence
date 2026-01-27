#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "ZombieStateFragment.generated.h"

UENUM(BlueprintType)
enum class EZombieLocoState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Walk UMETA(DisplayName = "Walk"),
	Run UMETA(DisplayName = "Run")
};

UENUM(BlueprintType)
enum class EZombieActionState : uint8
{
	None UMETA(DisplayName = "None"),
	Attack UMETA(DisplayName = "Attack"),
	HitReact UMETA(DisplayName = "HitReact"),
	Dead UMETA(DisplayName = "Dead")
};

USTRUCT(BlueprintType)
struct MYPROJECTTSECUENCE_API FZombieStateFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|State")
	EZombieLocoState Loco = EZombieLocoState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|State")
	EZombieActionState Action = EZombieActionState::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|State")
	bool bHasTarget = false;
};
