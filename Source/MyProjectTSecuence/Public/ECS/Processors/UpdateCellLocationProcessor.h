// Copyright 2024 MyProjectTSec

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "MassCommonFragments.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Core/GridConfig.h"
#include "ECS/Fragments/ZombiCoreFragment.h"
#include "ECS/Fragments/CellLocationFragment.h"
#include "UpdateCellLocationProcessor.generated.h"

/**
 * Processor que actualiza la ubicación en la cuadrícula de las entidades
 * basado en su posición en el mundo.
 */
UCLASS()
class MYPROJECTTSECUENCE_API UUpdateCellLocationProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UUpdateCellLocationProcessor();

    /** Configura las queries del processor */
    virtual void ConfigureQueries() override;

    /** Ejecuta el processor */
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
    /** Query para entidades con posición y que necesitan actualizar su ubicación en la cuadrícula */
    FMassEntityQuery EntityQuery;
};
