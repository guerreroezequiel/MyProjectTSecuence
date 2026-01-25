#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "TurboSequenceSolveProcessor.generated.h"

UCLASS()
class MYPROJECTTSECUENCE_API UTurboSequenceSolveProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UTurboSequenceSolveProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
	uint64 LastSolvedFrame = 0;
};
