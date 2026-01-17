// Copyright 2024 MyProjectTSec

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "GridSystem/FlowField/FlowFieldStorage.h"
#include "GridSystem/FlowField/FlowFieldTypes.h"
#include "ECS/Fragments/FlowReadFragment.h"
#include "ECS/Fragments/CellLocationFragment.h"
#include "ECS/Tags/ZombiTag.h"
#include "FlowDirReadPlayersProcessor.generated.h"

/**
 * Processor que lee la dirección del FlowField para las entidades.
 * Solo lee el Intent::Players para este MVP.
 */
UCLASS()
class MYPROJECTTSECUENCE_API UFlowDirReadPlayersProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UFlowDirReadPlayersProcessor();

    /** Configura las queries del processor */
    virtual void ConfigureQueries() override;

    /** Ejecuta el processor */
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
    /** Query para entidades que necesitan leer el FlowField */
    FMassEntityQuery EntityQuery;
};
