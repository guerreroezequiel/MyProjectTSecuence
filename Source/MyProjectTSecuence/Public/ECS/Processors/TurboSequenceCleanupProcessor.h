#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassCommonFragments.h"
#include "ECS/Fragments/TurboSequenceInstanceFragment.h"
#include "ECS/Tags/TSPendingCleanupTag.h"
#include "TurboSequenceCleanupProcessor.generated.h"

UCLASS()
class MYPROJECTTSECUENCE_API UTurboSequenceCleanupProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UTurboSequenceCleanupProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery CleanupQuery;
	void RemoveTSInstanceIfValid(UWorld* World, FTurboSequenceInstanceFragment& TS) const;
};
