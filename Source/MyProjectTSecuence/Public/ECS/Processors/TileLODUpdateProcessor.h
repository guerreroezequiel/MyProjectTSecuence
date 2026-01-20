#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassCommonFragments.h"
#include "ECS/Fragments/CellLocationFragment.h"
#include "ECS/Fragments/TileLODFragment.h"
#include "ECS/Tags/ZombiTag.h"
#include "TileLODUpdateProcessor.generated.h"

UCLASS()
class MYPROJECTTSECUENCE_API UTileLODUpdateProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UTileLODUpdateProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
