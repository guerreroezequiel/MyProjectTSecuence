#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "TurboSequenceSpawnProcessor.generated.h"

UCLASS()
class MYPROJECTTSECUENCE_API UTurboSequenceSpawnProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UTurboSequenceSpawnProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
