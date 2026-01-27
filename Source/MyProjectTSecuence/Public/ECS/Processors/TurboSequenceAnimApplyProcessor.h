#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "TurboSequenceAnimApplyProcessor.generated.h"

UCLASS()
class MYPROJECTTSECUENCE_API UTurboSequenceAnimApplyProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UTurboSequenceAnimApplyProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
