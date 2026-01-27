#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "ZombieStateUpdateProcessor.generated.h"

UCLASS()
class MYPROJECTTSECUENCE_API UZombieStateUpdateProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UZombieStateUpdateProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
