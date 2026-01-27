#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "AnimRequestFragment.generated.h"

class UAnimSequence;

USTRUCT(BlueprintType)
struct MYPROJECTTSECUENCE_API FAnimRequestFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Anim")
	TObjectPtr<UAnimSequence> DesiredAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Anim")
	float DesiredPlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Anim")
	bool bLoop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Anim")
	bool bDirty = true;
};
