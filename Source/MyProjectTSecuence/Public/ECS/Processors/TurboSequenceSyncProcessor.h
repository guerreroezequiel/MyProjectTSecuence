#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassCommonFragments.h"
#include "ECS/Fragments/TurboSequenceInstanceFragment.h"
#include "ECS/Tags/ZombiTag.h"
#include "TurboSequenceSyncProcessor.generated.h"

UCLASS()
class MYPROJECTTSECUENCE_API UTurboSequenceSyncProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UTurboSequenceSyncProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
