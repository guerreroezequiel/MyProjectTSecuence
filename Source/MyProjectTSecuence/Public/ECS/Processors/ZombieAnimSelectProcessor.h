#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "ZombieAnimSelectProcessor.generated.h"

 class UAnimSequence;

UCLASS()
class MYPROJECTTSECUENCE_API UZombieAnimSelectProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UZombieAnimSelectProcessor();

	UPROPERTY(EditAnywhere, Category = "Zombie|Anim")
	TObjectPtr<UAnimSequence> AnimIdle = nullptr;

	UPROPERTY(EditAnywhere, Category = "Zombie|Anim")
	TObjectPtr<UAnimSequence> AnimWalk = nullptr;

	UPROPERTY(EditAnywhere, Category = "Zombie|Anim")
	TObjectPtr<UAnimSequence> AnimRun = nullptr;

	UPROPERTY(EditAnywhere, Category = "Zombie|Anim")
	TObjectPtr<UAnimSequence> AnimDead = nullptr;

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
