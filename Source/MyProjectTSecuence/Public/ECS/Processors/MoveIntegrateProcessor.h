// Copyright 2024 MyProjectTSec

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "ECS/Fragments/FlowReadFragment.h"
#include "ECS/Fragments/MoveFragment.h"
#include "ECS/Fragments/ZombiCoreFragment.h"
#include "ECS/Tags/ZombiTag.h"
#include "MoveIntegrateProcessor.generated.h"

/**
 * Processor que maneja la integración del movimiento para las entidades
 * basado en su velocidad y dirección actual.
 */
UCLASS()
class MYPROJECTTSECUENCE_API UMoveIntegrateProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UMoveIntegrateProcessor();

    /** Configura las queries del processor */
    virtual void ConfigureQueries() override;

    /** Ejecuta el processor */
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
    /** Query para entidades con movimiento */
    FMassEntityQuery EntityQuery;
};
