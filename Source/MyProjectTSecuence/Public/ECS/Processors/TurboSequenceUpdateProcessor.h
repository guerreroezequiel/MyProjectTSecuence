#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "TurboSequenceUpdateProcessor.generated.h"

UCLASS()
class MYPROJECTTSECUENCE_API UTurboSequenceUpdateProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UTurboSequenceUpdateProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
