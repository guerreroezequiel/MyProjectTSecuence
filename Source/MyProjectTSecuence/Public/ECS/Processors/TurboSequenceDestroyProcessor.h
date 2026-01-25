#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "TurboSequenceDestroyProcessor.generated.h"

UCLASS()
class MYPROJECTTSECUENCE_API UTurboSequenceDestroyProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UTurboSequenceDestroyProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
